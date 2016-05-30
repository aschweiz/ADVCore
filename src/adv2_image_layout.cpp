/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_image_layout.h"
#include "utils.h"
#include "math.h"
#include <stdlib.h>

namespace AdvLib2
{

Adv2ImageLayout::Adv2ImageLayout(Adv2ImageSection* imageSection, unsigned int width, unsigned int height, unsigned char layoutId, const char* layoutType, const char* compression, unsigned char dataBpp, int keyFrame)
{
	m_ImageSection = imageSection;
	LayoutId = layoutId;
	Width = width;
	Height = height;
	Compression = NULL;
	DataBpp = dataBpp;

	InitialiseBuffers();

	AddOrUpdateTag("SECTION-DATA-COMPRESSION", compression);
}

void Adv2ImageLayout::InitialiseBuffers()
{
	SIGNS_MASK = new unsigned char(8);
	SIGNS_MASK[0] = 0x01;
	SIGNS_MASK[1] = 0x02;
	SIGNS_MASK[2] = 0x04;
	SIGNS_MASK[3] = 0x08;
	SIGNS_MASK[4] = 0x10;
	SIGNS_MASK[5] = 0x20;
	SIGNS_MASK[6] = 0x40;
	SIGNS_MASK[7] = 0x80;
	
	
	int signsBytesCount = (unsigned int)ceil(Width * Height / 8.0);
	
	if (Bpp == 8)
	{
		MaxFrameBufferSize	= Width * Height + 1 + 4 + signsBytesCount + 16;
	}
	else if (Bpp == 12)
	{
		MaxFrameBufferSize	= (Width * Height * 3 / 2) + 1 + 4 + 2 * ((Width * Height) % 2) + signsBytesCount + 16;
	}
	else if (Bpp == 16)
	{
		MaxFrameBufferSize = (Width * Height * 2) + 1 + 4 + signsBytesCount + 16;
	}
	else 
		MaxFrameBufferSize = Width * Height * 4 + 1 + 4 + 2 * signsBytesCount + 16;

	
	m_MaxSignsBytesCount = (unsigned int)ceil(Width * Height / 8.0);

	if (Bpp == 8)
		m_MaxPixelArrayLengthWithoutSigns = 1 + 4 + Width * Height;	
	else if (Bpp == 12)
		m_MaxPixelArrayLengthWithoutSigns = 1 + 4 + 3 * (Width * Height) / 2 + 2 * ((Width * Height) % 2);	
	else if (Bpp == 16)
		m_MaxPixelArrayLengthWithoutSigns = 1 + 4 + 2 * Width * Height;	
	else
		m_MaxPixelArrayLengthWithoutSigns = 1 + 4 + 4 * Width * Height;	
		
	m_MaxPixelArrayLengthWithoutSigns = 1 + 4 + 4 * Width * Height;	
	m_KeyFrameBytesCount = Width * Height * sizeof(unsigned short);
	
	m_PrevFramePixels = NULL;
	m_PrevFramePixelsTemp = NULL;
	m_PixelArrayBuffer = NULL;
	m_SignsBuffer = NULL;
	m_DecompressedPixels = NULL;	
	m_StateDecompress = NULL;
	
	m_PixelArrayBuffer = (unsigned char*)malloc(m_MaxPixelArrayLengthWithoutSigns + m_MaxSignsBytesCount);
	m_PrevFramePixels = (unsigned short*)malloc(m_KeyFrameBytesCount);		
	memset(m_PrevFramePixels, 0, m_KeyFrameBytesCount);
	
	m_PrevFramePixelsTemp = (unsigned short*)malloc(m_KeyFrameBytesCount);	
	m_SignsBuffer = (unsigned char*)malloc(m_MaxSignsBytesCount);
	m_DecompressedPixels = (char*)malloc(MaxFrameBufferSize);
	
	m_StateDecompress = (qlz_state_decompress *)malloc(sizeof(qlz_state_decompress));
	m_Lagarith16Decompressor = new Compressor(Width, Height);
}

Adv2ImageLayout::~Adv2ImageLayout()
{
	ResetBuffers();
}

void Adv2ImageLayout::ResetBuffers()
{
	if (NULL != m_PrevFramePixels)
		delete m_PrevFramePixels;		

	if (NULL != m_PrevFramePixelsTemp)
		delete m_PrevFramePixelsTemp;		

	if (NULL != m_PixelArrayBuffer)
		delete m_PixelArrayBuffer;

	if (NULL != m_SignsBuffer)
		delete m_SignsBuffer;

	if (NULL != m_DecompressedPixels)
		delete m_DecompressedPixels;
	
	if (NULL != m_StateDecompress)
		delete m_StateDecompress;	

	if (NULL != m_Lagarith16Decompressor)
		delete m_Lagarith16Decompressor;
		
	m_PrevFramePixels = NULL;
	m_PrevFramePixelsTemp = NULL;
	m_PixelArrayBuffer = NULL;
	m_SignsBuffer = NULL;
	m_DecompressedPixels = NULL;	
	m_StateDecompress = NULL;
	m_Lagarith16Decompressor = NULL;
}


void Adv2ImageLayout::AddOrUpdateTag(const char* tagName, const char* tagValue)
{
	map<string, string>::iterator curr = m_LayoutTags.begin();
	while (curr != m_LayoutTags.end()) 
	{
		const char* existingTagName = curr->first.c_str();
		
		if (0 == strcmp(existingTagName, tagName))
		{
			m_LayoutTags.erase(curr);
			break;
		}
		
		curr++;
	}
	
	m_LayoutTags.insert(make_pair(string(tagName), string(tagValue == NULL ? "" : tagValue)));

	if (0 == strcmp("DIFFCODE-BASE-FRAME", tagName))
	{
		if (0 == strcmp("KEY-FRAME", tagValue))
		{
			BaseFrameType = DiffCorrKeyFrame;
		}
		else if (0 == strcmp("PREV-FRAME", tagValue))
		{
			BaseFrameType = DiffCorrPrevFrame;
		}
	}
	
	if (0 == strcmp("DATA-LAYOUT", tagName))
	{
		m_BytesLayout = FullImageRaw;
		if (0 == strcmp("FULL-IMAGE-DIFFERENTIAL-CODING", tagValue)) m_BytesLayout = FullImageDiffCorrWithSigns;
		IsDiffCorrLayout = m_BytesLayout == FullImageDiffCorrWithSigns;
	}

	if (0 == strcmp("SECTION-DATA-COMPRESSION", tagName))
	{
		if (Compression == NULL) delete Compression;

		Compression = new char[strlen(tagValue) + 1];
		strcpy(const_cast<char*>(Compression), tagValue);

		m_UsesCompression = 0 != strcmp(tagValue, "UNCOMPRESSED");
	}
}

void Adv2ImageLayout::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 2;
	fwrite(&buffChar, 1, 1, pFile); /* Version */

	fwrite(&Bpp, 1, 1, pFile);	

	
	buffChar = (unsigned char)m_LayoutTags.size();
	fwrite(&buffChar, 1, 1, pFile);
	
	map<string, string>::iterator curr = m_LayoutTags.begin();
	while (curr != m_LayoutTags.end()) 
	{
		char* tagName = const_cast<char*>(curr->first.c_str());	
		WriteUTF8String(pFile, tagName);
		
		char* tagValue = const_cast<char*>(curr->second.c_str());	
		WriteUTF8String(pFile, tagValue);
		
		curr++;
	}		
}

}