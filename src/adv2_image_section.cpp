/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "adv2_image_section.h"
#include "utils.h"

namespace AdvLib2
{

#define UNINITIALIZED_LAYOUT_ID 0	
unsigned char m_PreviousLayoutId;
unsigned int m_NumFramesInThisLayoutId;

Adv2ImageSection::Adv2ImageSection(unsigned int width, unsigned int height, unsigned char dataBpp)
{
	Width = width;
	Height = height;
	DataBpp = dataBpp;
	
	m_PreviousLayoutId = UNINITIALIZED_LAYOUT_ID;
	m_NumFramesInThisLayoutId = 0;
	ByteOrder = ImageByteOrder::LittleEndian;
	UsesCRC = false;
	MaxPixelValue = 0;
	m_RGBorBGR = false;
}

Adv2ImageSection::~Adv2ImageSection()
{
	map<unsigned char, Adv2ImageLayout*>::iterator currIml = m_ImageLayouts.begin();
	while (currIml != m_ImageLayouts.end()) 
	{
		Adv2ImageLayout* imageLayout = currIml->second;
		delete imageLayout;
		
		currIml++;
	}
	
	m_ImageLayouts.empty();
}

Adv2ImageLayout* Adv2ImageSection::AddImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp)
{
	AdvLib2::Adv2ImageLayout* layout = new AdvLib2::Adv2ImageLayout(this, Width, Height, layoutId, layoutType, compression, layoutBpp); 
	m_ImageLayouts.insert(make_pair(layoutId, layout));
	return layout;
}

void Adv2ImageSection::AddOrUpdateTag(const char* tagName, const char* tagValue)
{
	map<string, string>::iterator curr = m_ImageTags.begin();
	while (curr != m_ImageTags.end()) 
	{
		const char* existingTagName = curr->first.c_str();
		
		if (0 == strcmp(existingTagName, tagName))
		{
			m_ImageTags.erase(curr);
			break;
		}
		
		curr++;
	}

	if (strcmp("IMAGE-BYTE-ORDER", tagName) == 0)
	{
		ByteOrder = ImageByteOrder::LittleEndian;

		if (strcmp("BIG-ENDIAN", tagValue) == 0)
			ByteOrder = ImageByteOrder::BigEndian;
	}

	if (strcmp("SECTION-DATA-REDUNDANCY-CHECK", tagName) == 0)
	{
		UsesCRC = strcmp("CRC32", tagValue) == 0;
	}

	if (strcmp("IMAGE-MAX-PIXEL-VALUE", tagName) == 0 && tagValue != nullptr)
	{
		MaxPixelValue = atoi(tagValue);
	}

	if (strcmp("IMAGE-BAYER-PATTERN", tagName) == 0 && tagValue != nullptr)
	{
		m_RGBorBGR = strcmp("RGB", tagValue) == 0 || strcmp("BGR", tagValue) == 0;
		IsColourImage = strcmp("MONOCHROME", tagValue) != 0;
		strcpy_s(ImageBayerPattern, tagValue);
	}
	
	m_ImageTags.insert(make_pair(string(tagName), string(tagValue == nullptr ? "" : tagValue)));
}

Adv2ImageSection::Adv2ImageSection(FILE* pFile)
{
	unsigned char version;
	advfread(&version, 1, 1, pFile); /* Version */

	advfread(&Width, 4, 1, pFile);
	advfread(&Height, 4, 1, pFile);
	advfread(&DataBpp, 1, 1, pFile);

	ByteOrder = ImageByteOrder::LittleEndian;
	UsesCRC = false;
	IsColourImage = false;
	MaxPixelValue = 0;
	m_RGBorBGR = false;

	unsigned char imageLayouts;
	advfread(&imageLayouts, 1, 1, pFile);

	for (int i = 0; i < imageLayouts; i++)
	{
		char layoutId;
		advfread(&layoutId, 1, 1, pFile);

		Adv2ImageLayout* imageLayout = new AdvLib2::Adv2ImageLayout(this, layoutId, pFile);
		m_ImageLayouts.insert(make_pair(layoutId, imageLayout));
	}

	unsigned char tagsCount;
	advfread(&tagsCount, 1, 1, pFile);

	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(pFile);
		char* tagValue = ReadUTF8String(pFile);

		AddOrUpdateTag(tagName, tagValue);
	}
}

void Adv2ImageSection::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 2;
	advfwrite(&buffChar, 1, 1, pFile); /* Version */

	
	advfwrite(&Width, 4, 1, pFile);
	advfwrite(&Height, 4, 1, pFile);
	advfwrite(&DataBpp, 1, 1, pFile);
	
	buffChar = (unsigned char)m_ImageLayouts.size();
	advfwrite(&buffChar, 1, 1, pFile);
	
	map<unsigned char, Adv2ImageLayout*>::iterator currIml = m_ImageLayouts.begin();
	while (currIml != m_ImageLayouts.end()) 
	{
		char layoutId = currIml->first;	
		advfwrite(&layoutId, 1, 1, pFile);
		
		Adv2ImageLayout* imageLayout = currIml->second;	
		imageLayout->WriteHeader(pFile);
		
		currIml++;
	}
	
	buffChar = (unsigned char)m_ImageTags.size();
	advfwrite(&buffChar, 1, 1, pFile);
	
	map<string, string>::iterator curr = m_ImageTags.begin();
	while (curr != m_ImageTags.end()) 
	{
		char* tagName = const_cast<char*>(curr->first.c_str());
		WriteUTF8String(pFile, tagName);
		
		char* tagValue = const_cast<char*>(curr->second.c_str());
		WriteUTF8String(pFile, tagValue);
		
		curr++;
	}
}

void Adv2ImageSection::BeginFrame()
{
	// Nothing special to do here
}

int m_MaxImageLayoutFrameBufferSize = -1;

int Adv2ImageSection::MaxFrameBufferSize()
{
	// Max frame buffer size is the max frame buffer size of the largest image layout
	if (m_MaxImageLayoutFrameBufferSize == -1)
	{
		map<unsigned char, Adv2ImageLayout*>::iterator curr = m_ImageLayouts.begin();
		while (curr != m_ImageLayouts.end()) 
		{
			int maxBuffSize = curr->second->MaxFrameBufferSize;
				
			if (m_MaxImageLayoutFrameBufferSize < maxBuffSize) 
				m_MaxImageLayoutFrameBufferSize = maxBuffSize;
			
			curr++;
		}		
	}
		
	return m_MaxImageLayoutFrameBufferSize;
}

unsigned char* Adv2ImageSection::GetDataBytes(unsigned char layoutId, unsigned short* currFramePixels, unsigned int *bytesCount, unsigned char pixelsBpp, enum GetByteOperation operation)
{
	Adv2ImageLayout* currentLayout = GetImageLayoutById(layoutId);
	
	if (m_PreviousLayoutId == layoutId)
		m_NumFramesInThisLayoutId++;
	else
	{
		m_NumFramesInThisLayoutId = 0;
	}
	
	unsigned char* pixels = currentLayout->GetDataBytes(currFramePixels, bytesCount, pixelsBpp, operation);	
	
	m_PreviousLayoutId = layoutId;
	
	return pixels;
}

AdvLib2::Adv2ImageLayout* Adv2ImageSection::GetImageLayoutById(unsigned char layoutId)
{
	map<unsigned char, Adv2ImageLayout*>::iterator curr = m_ImageLayouts.begin();
	while (curr != m_ImageLayouts.end()) 
	{
		unsigned char id =curr->first;
	
		if (id == layoutId)
			return curr->second;
			
		curr++;
	}
	
	return nullptr;
}

void Adv2ImageSection::GetDataFromDataBytes(unsigned char* data, unsigned int* pixels, int sectionDataLength, int startOffset)
{
	unsigned char* sectionData = data + startOffset;
	unsigned char layoutId = *sectionData;
	sectionData++;

	enum GetByteMode reservedUnusedField = (GetByteMode)*sectionData;
	sectionData++;

	Adv2ImageLayout* imageLayout = GetImageLayoutById(layoutId);	
	imageLayout->GetDataFromDataBytes(data, pixels, sectionDataLength - 2, startOffset + 2);
}

}