/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
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

Adv2ImageLayout* Adv2ImageSection::AddImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char bpp, int keyFrame)
{
	AdvLib2::Adv2ImageLayout* layout = new AdvLib2::Adv2ImageLayout(this, Width, Height, layoutId, layoutType, compression, bpp, keyFrame); 
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
	
	m_ImageTags.insert(make_pair(string(tagName), string(tagValue == NULL ? "" : tagValue)));
}

void Adv2ImageSection::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 2;
	fwrite(&buffChar, 1, 1, pFile); /* Version */

	
	fwrite(&Width, 4, 1, pFile);
	fwrite(&Height, 4, 1, pFile);
	fwrite(&DataBpp, 1, 1, pFile);	
	
	buffChar = (unsigned char)m_ImageLayouts.size();
	fwrite(&buffChar, 1, 1, pFile);
	
	map<unsigned char, Adv2ImageLayout*>::iterator currIml = m_ImageLayouts.begin();
	while (currIml != m_ImageLayouts.end()) 
	{
		char layoutId = currIml->first;	
		fwrite(&layoutId, 1, 1, pFile);
		
		Adv2ImageLayout* imageLayout = currIml->second;	
		imageLayout->WriteHeader(pFile);
		
		currIml++;
	}
	
	buffChar = (unsigned char)m_ImageTags.size();
	fwrite(&buffChar, 1, 1, pFile);
	
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

unsigned char* Adv2ImageSection::GetDataBytes(unsigned char layoutId, unsigned short* currFramePixels, unsigned int *bytesCount, char* byteMode, unsigned char pixelsBpp)
{
	Adv2ImageLayout* currentLayout = GetImageLayoutById(layoutId);
	
	if (m_PreviousLayoutId == layoutId)
		m_NumFramesInThisLayoutId++;
	else
	{
		m_NumFramesInThisLayoutId = 0;
		currentLayout->StartNewDiffCorrSequence();
	}
	
	enum GetByteMode mode = Normal;
	
	if (currentLayout->IsDiffCorrLayout)
	{
		bool isKeyFrame = (m_NumFramesInThisLayoutId % currentLayout->KeyFrame) == 0;
		bool diffCorrFromPrevFramePixels = isKeyFrame || currentLayout->BaseFrameType == DiffCorrPrevFrame;
		
		if (isKeyFrame)
		{
			// this is a key frame
			mode = KeyFrameBytes;		
		}
		else
		{
			// this is not a key frame, compute and save the diff corr
			mode = DiffCorrBytes;
		}
	}	
	
	unsigned char* pixels = currentLayout->GetDataBytes(currFramePixels, mode, bytesCount, pixelsBpp);
	
	
	m_PreviousLayoutId = layoutId;
	*byteMode = (char)mode;
	
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
	
	return NULL;
}


}