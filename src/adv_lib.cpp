/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <cstdint>

#include "adv_lib.h"
#include "adv_image_layout.h"
#include "adv_profiling.h"
#include "adv2_file.h"


char* g_CurrentAdvFile;
AdvLib::AdvFile* g_AdvFile;
bool g_FileStarted = false;

AdvLib2::Adv2File* g_Adv2File;

using namespace std;

char* AdvGetCurrentFilePath(void)
{
	return g_CurrentAdvFile;
}

unsigned int AdvGetFileVersion(const char* fileName)
{
	FILE* probe = advfopen(fileName, "rb");
	if (probe == 0) return 0;
	
	unsigned int buffInt;
	unsigned char buffChar;

	advfread(&buffInt, 4, 1, probe);
	advfread(&buffChar, 1, 1, probe);
	advfclose(probe);	
	
	if (buffInt != 0x46545346) return 0;
	return (unsigned int)buffChar;
}

int AdvOpenFile(const char* fileName)
{
	FILE* probe = advfopen(fileName, "rb");
	if (probe == 0) return 0;
	
	unsigned int buffInt;
	unsigned char buffChar;

	advfread(&buffInt, 4, 1, probe);
	advfread(&buffChar, 1, 1, probe);
	advfclose(probe);
	
	if (buffInt != 0x46545346) return 0;
	
	if (buffChar == 1)
	{
		if (NULL != g_AdvFile)
		{
			delete g_AdvFile;
			g_AdvFile = NULL;
		}
		
		g_FileStarted = false;
		
		int len = strlen(fileName);
		if (len > 0)
		{
			g_CurrentAdvFile = new char[len + 1];
			strncpy(g_CurrentAdvFile, fileName, len + 1);
		
			g_AdvFile = new AdvLib::AdvFile();
			int res = !g_AdvFile->LoadFile(fileName);
			if (res < 0)
			{
				delete g_AdvFile;
				g_AdvFile = NULL;
				return res;
			}
		}
		
		return 1;
	}
	else if (buffChar == 2)
	{
		if (NULL != g_Adv2File)
		{
			delete g_Adv2File;
			g_Adv2File = NULL;
		}
		
		g_FileStarted = false;
		
		int len = strlen(fileName);
		if (len > 0)
		{
			g_CurrentAdvFile = new char[len + 1];
			strncpy(g_CurrentAdvFile, fileName, len + 1);
		
			g_Adv2File = new AdvLib2::Adv2File();
			int res = !g_Adv2File->LoadFile(fileName);
			if (res < 0)
			{
				delete g_Adv2File;
				g_Adv2File = NULL;
				return res;
			}
		}
		
		return 2;
	}

	return 0;
}

unsigned int AdvCloseFile()
{
	unsigned int rv = 0;

	if (NULL != g_AdvFile)
	{
		g_AdvFile->CloseFile();
		delete g_AdvFile;
		g_AdvFile = NULL;
		rv += 1;
	}

	if (NULL != g_Adv2File)
	{
		g_Adv2File->CloseFile();
		delete g_Adv2File;
		g_Adv2File = NULL;
		rv += 2;
	}

	return rv;
}

void AdvVer1_NewFile(const char* fileName)
{
	AdvProfiling_ResetPerformanceCounters();
	AdvProfiling_StartProcessing();
	
    if (NULL != g_AdvFile)
	{
		delete g_AdvFile;
		g_AdvFile = NULL;		
	}
	
	if (NULL != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = NULL;
	}
	
	g_FileStarted = false;
	
	int len = strlen(fileName);	
	if (len > 0)
	{
		g_CurrentAdvFile = new char[len + 1];
		strncpy(g_CurrentAdvFile, fileName, len + 1);
	
		g_AdvFile = new AdvLib::AdvFile();	
	}
	AdvProfiling_EndProcessing();
}

void AdvVer1_EndFile()
{
	if (NULL != g_AdvFile)
	{
		g_AdvFile->EndFile();
		
		delete g_AdvFile;
		g_AdvFile = NULL;		
	}
	
	if (NULL != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = NULL;
	}
	
	g_FileStarted = false;
}

void AdvVer1_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp)
{
	AdvProfiling_StartProcessing();
	AdvLib::AdvImageSection* imageSection = new AdvLib::AdvImageSection(width, height, dataBpp);
	g_AdvFile->AddImageSection(imageSection);
	AdvProfiling_EndProcessing();
}

void AdvVer1_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char bpp, int keyFrame, const char* diffCorrFromBaseFrame)
{	
	AdvProfiling_StartProcessing();
	AdvLib::AdvImageLayout* imageLayout = g_AdvFile->ImageSection->AddImageLayout(layoutId, layoutType, compression, bpp, keyFrame);
	if (diffCorrFromBaseFrame != NULL)
		imageLayout->AddOrUpdateTag("DIFFCODE-BASE-FRAME", diffCorrFromBaseFrame);
		
	AdvProfiling_EndProcessing();
}

unsigned int AdvVer1_DefineStatusSectionTag(const char* tagName, int tagType)
{
	AdvProfiling_StartProcessing();
	unsigned int statusTagId = g_AdvFile->StatusSection->DefineTag(tagName, (AdvTagType)tagType);
	AdvProfiling_EndProcessing();
	return statusTagId;
}

unsigned int AdvVer1_AddFileTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();	
	unsigned int fileTagId = g_AdvFile->AddFileTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
	return fileTagId;
}

void AdvVer1_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	return g_AdvFile->ImageSection->AddOrUpdateTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
}

bool AdvVer1_BeginFrame(long long timeStamp, unsigned int elapsedTime, unsigned int exposure)
{
	AdvProfiling_StartProcessing();
	if (!g_FileStarted)
	{
		bool success = g_AdvFile->BeginFile(g_CurrentAdvFile);
		if (success)
		{
			g_FileStarted = true;	
		}
		else
		{
			g_FileStarted = false;
			return false;
		}		
	}
	
	g_AdvFile->BeginFrame(timeStamp, elapsedTime, exposure);
	AdvProfiling_EndProcessing();
	return true;
}

void AdvVer1_FrameAddImageBytes(unsigned char layoutId,  unsigned char* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameImage(layoutId, (unsigned short*)pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddImage(unsigned char layoutId,  unsigned short* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameImage(layoutId, pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTag(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{	
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagMessage(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt8(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_FrameAddStatusTag32(unsigned int tagIndex, unsigned long tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt32(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag64(unsigned int tagIndex, long long tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt64(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer1_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagUInt16(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue)
{
	AdvProfiling_StartProcessing();
	g_AdvFile->AddFrameStatusTagReal(tagIndex, tagValue);
	AdvProfiling_EndProcessing();	
}

void AdvVer1_EndFrame()
{
	AdvProfiling_StartProcessing();
	g_AdvFile->EndFrame();
	AdvProfiling_EndProcessing();
}

void GetLibraryVersion(char* version)
{
	strcpy(version, CORE_VERSION);
}

int GetLibraryBitness()
{
	return sizeof(long) * 8;
}

void GetLibraryPlatformId(char* platform)
{
#ifdef MSVC
	#if INTPTR_MAX == INT32_MAX
		strcpy(platform, "MS VC++, x86, Windows");
	#elif INTPTR_MAX == INT64_MAX
		strcpy(platform, "MS VC++, AMD64, Windows");
	#endif
#elif __linux__
	strcpy(platform, "Linux");
#elif __APPLE__
	strcpy(platform, "OSX");
#elif __GNUC__ || __GNUG__
	#if __x86_64__ || __ppc64__ || _WIN64
		strcpy(platform, "GNU GCC/G++, AMD64, Windows");
	#else
		strcpy(platform, "GNU GCC/G++, x86, Windows");
	#endif	
#else
	strcpy(platform, "Unknown");
#endif
}

void AdvVer2_NewFile(const char* fileName)
{
	AdvProfiling_ResetPerformanceCounters();
	AdvProfiling_StartProcessing();
	
    if (NULL != g_Adv2File)
	{
		delete g_Adv2File;
		g_Adv2File = NULL;
	}
	
	if (NULL != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = NULL;
	}
	
	g_FileStarted = false;
	
	int len = strlen(fileName);
	if (len > 0)
	{
		g_CurrentAdvFile = new char[len + 1];
		strncpy(g_CurrentAdvFile, fileName, len + 1);
	
		g_Adv2File = new AdvLib2::Adv2File();
	}
	AdvProfiling_EndProcessing();
}

void AdvVer2_SetTimingPrecision(__int64 mainClockFrequency, long mainStreamAccuracy, __int64 calibrationClockFrequency, long calibrationStreamAccuracy)
{
	if (NULL != g_Adv2File)
	{
		g_Adv2File->SetTimingPrecision(mainClockFrequency, mainStreamAccuracy, calibrationClockFrequency, calibrationStreamAccuracy);
	}
}

void AdvVer2_EndFile()
{
	if (NULL != g_Adv2File)
	{
		g_Adv2File->EndFile();
		
		delete g_Adv2File;
		g_Adv2File = NULL;
	}
	
	if (NULL != g_CurrentAdvFile)
	{
		delete g_CurrentAdvFile;
		g_CurrentAdvFile = NULL;
	}
	
	g_FileStarted = false;
}

unsigned int AdvVer2_AddMainStreamTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	int tagId = g_Adv2File->AddMainStreamTag(tagName, tagValue);
	AdvProfiling_EndProcessing();

	return tagId;
}

unsigned int AdvVer2_AddCalibrationStreamTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	int tagId = g_Adv2File->AddCalibrationStreamTag(tagName, tagValue);
	AdvProfiling_EndProcessing();

	return tagId;
}

bool AdvVer2_BeginFrame(unsigned int streamId, __int64 startFrameTicks, __int64 endFrameTicks,__int64 elapsedTicksSinceFirstFrame)
{
	AdvProfiling_StartProcessing();
	if (!g_FileStarted)
	{
		bool success = g_Adv2File->BeginFile(g_CurrentAdvFile);
		if (success)
		{
			g_FileStarted = true;	
		}
		else
		{
			g_FileStarted = false;
			return false;
		}		
	}
	
	g_Adv2File->BeginFrame(streamId, startFrameTicks, endFrameTicks, elapsedTicksSinceFirstFrame);
	AdvProfiling_EndProcessing();
	return true;
}

void AdvVer2_EndFrame()
{
	AdvProfiling_StartProcessing();
	g_Adv2File->EndFrame();
	AdvProfiling_EndProcessing();
}

void AdvVer2_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp)
{
	AdvProfiling_StartProcessing();
	AdvLib2::Adv2ImageSection* imageSection = new AdvLib2::Adv2ImageSection(width, height, dataBpp);
	g_Adv2File->AddImageSection(imageSection);
	AdvProfiling_EndProcessing();
}

void AdvVer2_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp, int keyFrame, const char* diffCorrFromBaseFrame)
{
	AdvProfiling_StartProcessing();
	AdvLib2::Adv2ImageLayout* imageLayout = g_Adv2File->ImageSection->AddImageLayout(layoutId, layoutType, compression, layoutBpp, keyFrame);
	if (diffCorrFromBaseFrame != NULL)
		imageLayout->AddOrUpdateTag("DIFFCODE-BASE-FRAME", diffCorrFromBaseFrame);
		
	AdvProfiling_EndProcessing();
}

unsigned int AdvVer2_DefineStatusSectionTag(const char* tagName, int tagType)
{
	AdvProfiling_StartProcessing();
	unsigned int statusTagId = g_Adv2File->StatusSection->DefineTag(tagName, (AdvTagType)tagType);
	AdvProfiling_EndProcessing();
	return statusTagId;
}

unsigned int AdvVer2_AddFileTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	unsigned int fileTagId = g_Adv2File->AddFileTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
	return fileTagId;
}

void AdvVer2_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	return g_Adv2File->ImageSection->AddOrUpdateTag(tagName, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagUTF8String(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUTF8String(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagMessage(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt8(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt16(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagReal(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag32(unsigned int tagIndex, unsigned long tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt32(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddStatusTag64(unsigned int tagIndex, __int64 tagValue)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameStatusTagUInt64(tagIndex, tagValue);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameImage(layoutId, pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer2_FrameAddImageBytes(unsigned char layoutId, unsigned char* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartProcessing();
	g_Adv2File->AddFrameImage(layoutId, (unsigned short*)pixels, pixelsBpp);
	AdvProfiling_EndProcessing();
}

void AdvVer2_GetMainStreamInfo(long* numFrames, __int64* mainClockFrequency, long* mainStreamAccuracy)
{
	g_Adv2File->GetMainStreamInfo(numFrames, mainClockFrequency, mainStreamAccuracy);
}

void AdvVer2_GetCalibrationStreamInfo(long* numFrames, __int64* calibrationClockFrequency, long* calibrationStreamAccuracy)
{
	g_Adv2File->GetCalibrationStreamInfo(numFrames, calibrationClockFrequency, calibrationStreamAccuracy);
}

