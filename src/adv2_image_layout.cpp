/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "adv2_image_layout.h"
#include "utils.h"
#include "math.h"
#include <stdlib.h>
#include "adv_profiling.h"

namespace AdvLib2
{

Adv2ImageLayout::Adv2ImageLayout(Adv2ImageSection* imageSection, unsigned int width, unsigned int height, unsigned char layoutId, const char* layoutType, const char* compression, unsigned char dataBpp, int keyFrame)
{
	m_ImageSection = imageSection;
	LayoutId = layoutId;
	Width = width;
	Height = height;
	Compression = nullptr;
	DataBpp = dataBpp;

	AddOrUpdateTag("DATA-LAYOUT", layoutType);
	AddOrUpdateTag("SECTION-DATA-COMPRESSION", compression);

	Compression = new char[strlen(compression) + 1];
	strcpy_s(const_cast<char*>(Compression), strlen(compression) + 1, compression);
	m_UsesCompression = 0 != strcmp(compression, "UNCOMPRESSED");
	m_UsesLagarith16Compression = 0 != strcmp(compression, "LAGARITH16");
	
	if (keyFrame > 0)
	{
		char keyFrameStr [5];
		_snprintf_s(keyFrameStr, 5, "%d", keyFrame);
		AddOrUpdateTag("DIFFCODE-KEY-FRAME-FREQUENCY", keyFrameStr);
		AddOrUpdateTag("DIFFCODE-BASE-FRAME", "KEY-FRAME");
	}

	InitialiseBuffers();
}

Adv2ImageLayout::Adv2ImageLayout(Adv2ImageSection* imageSection, unsigned int width, unsigned int height, char layoutId, unsigned char dataBpp, FILE* pFile)
{
	m_ImageSection = imageSection;
	LayoutId = layoutId;
	Width = width;
	Height = height;
	DataBpp = dataBpp;

	m_PrevFramePixels = nullptr;
	m_PrevFramePixelsTemp = nullptr;
	m_PixelArrayBuffer = nullptr;
	m_SignsBuffer = nullptr;
	m_CompressedPixels = nullptr;
	m_StateCompress = nullptr;
	m_Lagarith16Compressor = nullptr;

	m_UsesCompression = false;
	m_UsesLagarith16Compression = false;

	unsigned char version;
	advfread(&version, 1, 1, pFile); /* Version */

	advfread(&Bpp, 1, 1, pFile);

	unsigned char tagsCount;
	advfread(&tagsCount, 1, 1, pFile);

	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(pFile);
		char* tagValue = ReadUTF8String(pFile);

		m_LayoutTags.insert(make_pair(tagName, tagValue));

		if (strcmp("SECTION-DATA-COMPRESSION", tagName) == 0)
		{
			Compression = new char[strlen(tagValue) + 1];
			//strcpy(const_cast<char*>(Compression), tagValue);
			strcpy_s(const_cast<char*>(Compression), strlen(tagValue) + 1, tagValue);

			m_UsesCompression = 0 != strcmp(Compression, "UNCOMPRESSED");
			m_UsesLagarith16Compression = 0 != strcmp(Compression, "LAGARITH16");
		}
	}

	InitialiseBuffers();
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

	// In accordance with Lagarith16 specs
	if (m_UsesLagarith16Compression) MaxFrameBufferSize += 0x20000;

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
	
	m_PrevFramePixels = nullptr;
	m_PrevFramePixelsTemp = nullptr;
	m_PixelArrayBuffer = nullptr;
	m_SignsBuffer = nullptr;
	m_CompressedPixels = nullptr;
	m_StateCompress = nullptr;
	m_Lagarith16Compressor = nullptr;

	m_PixelArrayBuffer = (unsigned char*)malloc(m_MaxPixelArrayLengthWithoutSigns + m_MaxSignsBytesCount);
	m_PrevFramePixels = (unsigned short*)malloc(m_KeyFrameBytesCount);
	memset(m_PrevFramePixels, 0, m_KeyFrameBytesCount);
	
	m_PrevFramePixelsTemp = (unsigned short*)malloc(m_KeyFrameBytesCount);
	m_SignsBuffer = (unsigned char*)malloc(m_MaxSignsBytesCount);
	m_CompressedPixels = (char*)malloc(MaxFrameBufferSize);

	if (m_UsesCompression)
	{
		m_StateCompress = (qlz_state_compress *)malloc(sizeof(qlz_state_compress));

		if (m_UsesLagarith16Compression)
			m_Lagarith16Compressor = new Compressor(Width, Height);
	}
}

Adv2ImageLayout::~Adv2ImageLayout()
{
	ResetBuffers();

	if (nullptr != Compression)
	{
		delete Compression;
		Compression = nullptr;
	}
}

void Adv2ImageLayout::ResetBuffers()
{
	if (nullptr != m_PrevFramePixels)
		delete m_PrevFramePixels;

	if (nullptr != m_PrevFramePixelsTemp)
		delete m_PrevFramePixelsTemp;

	if (nullptr != m_PixelArrayBuffer)
		delete m_PixelArrayBuffer;

	if (nullptr != m_SignsBuffer)
		delete m_SignsBuffer;

	if (nullptr != m_CompressedPixels)
		delete m_CompressedPixels;
	
	if (nullptr != m_StateCompress)
		delete m_StateCompress;

	if (nullptr != m_Lagarith16Compressor)
		delete m_Lagarith16Compressor;
		
	m_PrevFramePixels = nullptr;
	m_PrevFramePixelsTemp = nullptr;
	m_PixelArrayBuffer = nullptr;
	m_SignsBuffer = nullptr;
	m_CompressedPixels = nullptr;
	m_StateCompress = nullptr;
	m_Lagarith16Compressor = nullptr;
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
	
	m_LayoutTags.insert(make_pair(string(tagName), string(tagValue == nullptr ? "" : tagValue)));

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
		if (Compression == nullptr) delete Compression;

		Compression = new char[strlen(tagValue) + 1];
		strcpy_s(const_cast<char*>(Compression), strlen(tagValue) + 1, tagValue);

		m_UsesCompression = 0 != strcmp(tagValue, "UNCOMPRESSED");
	}
}

void Adv2ImageLayout::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 2;
	advfwrite(&buffChar, 1, 1, pFile); /* Version */

	advfwrite(&Bpp, 1, 1, pFile);

	buffChar = (unsigned char)m_LayoutTags.size();
	advfwrite(&buffChar, 1, 1, pFile);
	
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

void Adv2ImageLayout::StartNewDiffCorrSequence()
{
   //TODO: Reset the prev frame buffer (do we need to do anything??)
}

unsigned char* Adv2ImageLayout::GetDataBytes(unsigned short* currFramePixels, enum GetByteMode mode, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{
	unsigned char* bytesToCompress;
	
	if (m_BytesLayout == FullImageDiffCorrWithSigns)
		bytesToCompress = GetFullImageDiffCorrWithSignsDataBytes(currFramePixels, mode, bytesCount, dataPixelsBpp);
	else if (m_BytesLayout == FullImageRaw)
		bytesToCompress = GetFullImageRawDataBytes(currFramePixels, bytesCount, dataPixelsBpp);

	
	if (0 == strcmp(Compression, "QUICKLZ"))
	{
		unsigned int frameSize = 0;
				
		AdvProfiling_StartFrameCompression();

		// compress and write result 
		size_t len2 = qlz_compress(bytesToCompress, m_CompressedPixels, *bytesCount, m_StateCompress); 		
		
		AdvProfiling_EndFrameCompression();
	
		*bytesCount = (unsigned int)len2;
		return (unsigned char*)(m_CompressedPixels);
	}
	if (0 == strcmp(Compression, "LAGARITH16"))
	{
		*bytesCount = m_Lagarith16Compressor->CompressData(currFramePixels, m_CompressedPixels);
		return (unsigned char*)(m_CompressedPixels);
	}
	else if (0 == strcmp(Compression, "UNCOMPRESSED"))
	{
		return bytesToCompress;
	}
	
		
	return nullptr;
}



unsigned char* Adv2ImageLayout::GetFullImageDiffCorrWithSignsDataBytes(unsigned short* currFramePixels, enum GetByteMode mode, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{
	bool isKeyFrame = mode == KeyFrameBytes;
	bool diffCorrFromPrevFramePixels = isKeyFrame || this->BaseFrameType == DiffCorrPrevFrame;
	
	if (diffCorrFromPrevFramePixels)
	{
		// STEP1 from maintaining the old pixels for DiffCorr
		if (mode == DiffCorrBytes)
			memcpy(&m_PrevFramePixelsTemp[0], &currFramePixels[0], m_KeyFrameBytesCount);
		else
			memcpy(&m_PrevFramePixels[0], &currFramePixels[0], m_KeyFrameBytesCount);
	}	

    // NOTE: The CRC computation is a huge overhead and is currently disabled
	//unsigned int pixelsCrc = ComputePixelsCRC32(currFramePixels);
	unsigned int pixelsCrc = 0;
	
	if (mode == KeyFrameBytes)
	{
		*bytesCount = 0;
	}
	else if (mode == DiffCorrBytes)
	{					
		*bytesCount = 0;
	
		AdvProfiling_StartTestingOperation();	

		unsigned int* pCurrFramePixels = (unsigned int*)currFramePixels;
		unsigned int* pPrevFramePixels = (unsigned int*)m_PrevFramePixels;
		for (unsigned int j = 0; j < Height; ++j)
		{
			for (unsigned int i = 0; i < Width / 2; ++i)
			{
				int wordCurr = (int)*pCurrFramePixels;
				int wordOld = (int)*pPrevFramePixels;
				
				unsigned int pixLo = (unsigned int)((unsigned short)((wordCurr & 0xFFFF) - (wordOld & 0xFFFF)));
				unsigned int pixHi = (unsigned int)((unsigned short)(((wordCurr & 0xFFFF0000) >> 16) - ((wordOld & 0xFFFF0000) >> 16)));
				
				*pCurrFramePixels = (pixHi << 16) + pixLo;
				
				pCurrFramePixels++;
				pPrevFramePixels++;
			}
		}
		AdvProfiling_EndTestingOperation();
	}
	
	if (diffCorrFromPrevFramePixels && mode == DiffCorrBytes)
		// STEP2 from maintaining the old pixels for DiffCorr
		memcpy(&m_PrevFramePixels[0], &m_PrevFramePixelsTemp[0], m_KeyFrameBytesCount);
	
	if (Bpp == 12)
	{		
		GetDataBytes12Bpp(currFramePixels, mode, pixelsCrc, bytesCount, dataPixelsBpp);
		return m_PixelArrayBuffer;
	}
	else if (Bpp = 16)
	{
		GetDataBytes16Bpp(currFramePixels, mode, pixelsCrc, bytesCount, dataPixelsBpp);
		return m_PixelArrayBuffer;
	}
	else
	{
		*bytesCount = 0;
		return nullptr;
	}
}

unsigned char* Adv2ImageLayout::GetFullImageRawDataBytes(unsigned short* currFramePixels, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{
	int buffLen = 0;
	if (dataPixelsBpp == 16)
	{
		buffLen = Width * Height * 2;
		memcpy(&m_PixelArrayBuffer[0], &currFramePixels[0], buffLen);		
	}
	else if (dataPixelsBpp == 8)
	{
		buffLen = Width * Height;
		memcpy(&m_PixelArrayBuffer[0], &currFramePixels[0], buffLen);
	}
	else
		// "12Bpp not supported in Raw layout"
		throw new exception();
	
	*bytesCount = buffLen;
	return m_PixelArrayBuffer;
}

void Adv2ImageLayout::GetDataBytes12BppIndex12BppWords(unsigned short* pixels, enum GetByteMode mode, unsigned int pixelsCRC32, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{	
	// NOTE: This code has never been tested or used !!!

	// Flags: 0 - no key frame used, 1 - key frame follows, 2 - diff corr data follows
	bool isKeyFrame = mode == KeyFrameBytes;
	bool noKeyFrameUsed = mode == Normal;
	bool isDiffCorrFrame = mode == DiffCorrBytes;

	// Every 2 12-bit values can be encoded in 3 bytes
	// xxxxxxxx|xxxxyyyy|yyyyyyy

	//int arrayLength = 1 + 4 + 3 * (Width * Height) / 2 + 2 * ((Width * Height) % 2) + *bytesCount;

	//unsigned char *imageData = (unsigned char*)malloc(arrayLength);
	
	//int signsBytesCnt = *bytesCount;
	
	unsigned int bytesCounter = *bytesCount;
	
	m_PixelArrayBuffer[0] = noKeyFrameUsed ? (unsigned char)0 : (isKeyFrame ? (unsigned char)1 : (unsigned char)2);
	bytesCounter++;
		
	unsigned int* pPixelArrayWords =  (unsigned int*)(&m_PixelArrayBuffer[0] + bytesCounter);
	unsigned int* pPixels = (unsigned int*)pixels;
	
	int counter = 0;
	int pixel8GroupCount = Height * Width / 8;
	for (int idx = 0; idx < pixel8GroupCount; ++idx)
	{
		unsigned int word1 = *pPixels;
		unsigned int word2 = *pPixels;pPixels++;
		unsigned int word3 = *pPixels;pPixels++;
		unsigned int word4 = *pPixels;pPixels++;
				
		//         word1                 word2                 word3                 word4
		// | 00aa aaaa 00bb bbbb | 00cc cccc 00dd dddd | 00ee eeee 00ff ffff | 00gg gggg 00hh hhhh|
        // | aaaa aabb bbbb cccc | ccdd dddd eeee eeff | ffff gggg gghh hhhh |
		//       encoded1               encoded2             encoded3
		
		unsigned int encodedPixelWord1 = ((word1 << 4) && 0xFFF00000) + ((word1 << 8) && 0x000FFF00) + (word2 >> 20);
		unsigned int encodedPixelWord2 = ((word2 << 12) && 0xF0000000) + (word2 << 16) + ((word3 >> 12) && 0x0000FFF0)+ ((word3 >> 8) && 0x0000000F);
		unsigned int encodedPixelWord3 = (word4 << 24) + ((word4 >> 4) && 0x00FFF000) + (word4 && 0x00000FFF);
		
		*pPixelArrayWords = encodedPixelWord1;pPixelArrayWords++;
		*pPixelArrayWords = encodedPixelWord2;pPixelArrayWords++;
		*pPixelArrayWords = encodedPixelWord3;pPixelArrayWords++;
		
		bytesCounter += 12;
	};

	*pPixelArrayWords = pixelsCRC32; pPixelArrayWords++;	
	(*bytesCount) = bytesCounter + 4;

	//if (isDiffCorrFrame)
	//	memcpy(&m_PixelArrayBuffer[1], &m_SignsBuffer[0], signsBytesCnt);
}

void Adv2ImageLayout::GetDataBytes12BppIndex16BppWords(unsigned short* pixels, enum GetByteMode mode, unsigned int pixelsCRC32, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{	
	// Flags: 0 - no key frame used, 1 - key frame follows, 2 - diff corr data follows
	bool isKeyFrame = mode == KeyFrameBytes;
	bool noKeyFrameUsed = mode == Normal;
	bool isDiffCorrFrame = mode == DiffCorrBytes;

	// Every 2 12-bit values can be encoded in 3 bytes
	// xxxxxxxx|xxxxyyyy|yyyyyyy

	//int arrayLength = 1 + 4 + 3 * (Width * Height) / 2 + 2 * ((Width * Height) % 2) + *bytesCount;

	//unsigned char *imageData = (unsigned char*)malloc(arrayLength);
	
	//int signsBytesCnt = *bytesCount;
	
	unsigned int bytesCounter = *bytesCount;
	
	m_PixelArrayBuffer[0] = noKeyFrameUsed ? (unsigned char)0 : (isKeyFrame ? (unsigned char)1 : (unsigned char)2);
	bytesCounter++;
		
	unsigned int* pPixelArrayWords =  (unsigned int*)(&m_PixelArrayBuffer[0] + bytesCounter);
	unsigned int* pPixels = (unsigned int*)pixels;
	
	int counter = 0;
	int pixel8GroupCount = Height * Width / 8;
	for (int idx = 0; idx < pixel8GroupCount; ++idx)
	{
		unsigned int word1 = *pPixels;
		unsigned int word2 = *pPixels;pPixels++;
		unsigned int word3 = *pPixels;pPixels++;
		unsigned int word4 = *pPixels;pPixels++;
		
		//(int)(pixels[x + y * Width] * 4095 / 65535)
		
		//         word1                 word2                 word3                 word4
		// | 00aa aaaa 00bb bbbb | 00cc cccc 00dd dddd | 00ee eeee 00ff ffff | 00gg gggg 00hh hhhh|
        // | aaaa aabb bbbb cccc | ccdd dddd eeee eeff | ffff gggg gghh hhhh |
		//       encoded1               encoded2             encoded3
		
		unsigned int encodedPixelWord1 = ((word1 << 4) && 0xFFF00000) + ((word1 << 8) && 0x000FFF00) + (word2 >> 20);
		unsigned int encodedPixelWord2 = ((word2 << 12) && 0xF0000000) + (word2 << 16) + ((word3 >> 12) && 0x0000FFF0)+ ((word3 >> 8) && 0x0000000F);
		unsigned int encodedPixelWord3 = (word4 << 24) + ((word4 >> 4) && 0x00FFF000) + (word4 && 0x00000FFF);
		
		*pPixelArrayWords = encodedPixelWord1;pPixelArrayWords++;
		*pPixelArrayWords = encodedPixelWord2;pPixelArrayWords++;
		*pPixelArrayWords = encodedPixelWord3;pPixelArrayWords++;
		
		bytesCounter += 12;
	};

	*pPixelArrayWords = pixelsCRC32; pPixelArrayWords++;	
	(*bytesCount) = bytesCounter + 4;

	//if (isDiffCorrFrame)
	//	memcpy(&m_PixelArrayBuffer[1], &m_SignsBuffer[0], signsBytesCnt);
}

void Adv2ImageLayout::GetDataBytes12BppIndexBytes(unsigned short* pixels, enum GetByteMode mode, unsigned int pixelsCRC32, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{	
	// Flags: 0 - no key frame used, 1 - key frame follows, 2 - diff corr data follows
	bool isKeyFrame = mode == KeyFrameBytes;
	bool noKeyFrameUsed = mode == Normal;
	bool isDiffCorrFrame = mode == DiffCorrBytes;

	// Every 2 12-bit values can be encoded in 3 bytes
	// xxxxxxxx|xxxxyyyy|yyyyyyy

	unsigned int bytesCounter = *bytesCount;
	
	//m_PixelArrayBuffer[0] = noKeyFrameUsed ? (unsigned char)0 : (isKeyFrame ? (unsigned char)1 : (unsigned char)2);
	//bytesCounter++;
		
	int counter = 0;
	for (unsigned int y = 0; y < Height; ++y)
	{
		for (unsigned int x = 0; x < Width; ++x)
		{
			unsigned short value =  dataPixelsBpp == 12 
				? (unsigned short)(pixels[x + y * Width] & 0xFFF)
				: (unsigned short)(pixels[x + y * Width] >> 4);
				
			counter++;

			switch (counter % 2)
			{
				case 1:
					m_PixelArrayBuffer[bytesCounter] = (unsigned char)(value >> 4);
					bytesCounter++;
					
					m_PixelArrayBuffer[bytesCounter] = (unsigned char)((value & 0x0F) << 4);
					break;

				case 0:
					m_PixelArrayBuffer[bytesCounter] += (unsigned char)(value >> 8);
					bytesCounter++;
					m_PixelArrayBuffer[bytesCounter] = (unsigned char)(value & 0xFF);
					bytesCounter++;
					break;
			}
		}
	}

	//m_PixelArrayBuffer[bytesCounter] = (unsigned char)(pixelsCRC32 & 0xFF);
	//m_PixelArrayBuffer[bytesCounter + 1] = (unsigned char)((pixelsCRC32 >> 8) & 0xFF);
	//m_PixelArrayBuffer[bytesCounter + 2] = (unsigned char)((pixelsCRC32 >> 16) & 0xFF);
	//m_PixelArrayBuffer[bytesCounter + 3] = (unsigned char)((pixelsCRC32 >> 24) & 0xFF);
	(*bytesCount) = bytesCounter;
}

void Adv2ImageLayout::GetDataBytes12Bpp(unsigned short* pixels, enum GetByteMode mode, unsigned int pixelsCRC32, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{
	GetDataBytes12BppIndexBytes(pixels, mode, pixelsCRC32, bytesCount, dataPixelsBpp);
}

void Adv2ImageLayout::GetDataBytes16Bpp(unsigned short* pixels, enum GetByteMode mode, unsigned int pixelsCRC32, unsigned int *bytesCount, unsigned char dataPixelsBpp)
{
	/*
	// Flags: 0 - no key frame used, 1 - key frame follows, 2 - diff corr data follows
	bool isKeyFrame = mode == KeyFrameBytes;
	bool noKeyFrameUsed = mode == Normal;
	bool isDiffCorrFrame = mode == DiffCorrBytes;
	
	//int arrayLength = 1 + 4 + 2 * Width * Height + *bytesCount;

	//unsigned char *imageData = (unsigned char*)malloc(arrayLength);
	
	int signsBytesCnt = *bytesCount;
	
	unsigned int bytesCounter = *bytesCount;
	
	m_PixelArrayBuffer[0] = noKeyFrameUsed ? (unsigned char)0 : (isKeyFrame ? (unsigned char)1 : (unsigned char)2);
	bytesCounter++;

	for (int y = 0; y < Height; ++y)
	{
		for (int x = 0; x < Width; ++x)
		{
			unsigned char lo = (unsigned char)(pixels[x + y * Width] & 0xFF);
			unsigned char hi = (unsigned char)(pixels[x + y * Width] >> 8);

			m_PixelArrayBuffer[bytesCounter] = hi;
			bytesCounter++;
			m_PixelArrayBuffer[bytesCounter] = lo;
			bytesCounter++;
		}
	}

	m_PixelArrayBuffer[bytesCounter] = (unsigned char)(pixelsCRC32 & 0xFF);
	m_PixelArrayBuffer[bytesCounter + 1] = (unsigned char)((pixelsCRC32 >> 8) & 0xFF);
	m_PixelArrayBuffer[bytesCounter + 2] = (unsigned char)((pixelsCRC32 >> 16) & 0xFF);
	m_PixelArrayBuffer[bytesCounter + 3] = (unsigned char)((pixelsCRC32 >> 24) & 0xFF);
	*bytesCount = bytesCounter + 4;

	if (isDiffCorrFrame)
		memcpy(&m_PixelArrayBuffer[1], &m_SignsBuffer[0], signsBytesCnt);
	*/
}

}