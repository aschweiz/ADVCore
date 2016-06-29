/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>
#include <time.h>

void WriteString(FILE* pFile, const char* str);
void WriteUTF8String(FILE* pFile, const char* str);

char* ReadString(FILE* pFile);
char* ReadUTF8String(FILE* pFile);

enum AdvTagType
{
	UInt8 = 0,
	UInt16 = 1,
	UInt32 = 2,
	ULong64 = 3,
	Real = 4, // IEEE/REAL*4
	AnsiString255 = 5,
	List16OfAnsiString255 = 6
};

enum Adv2TagType
{
	Int8 = 0,
	Int16 = 1,
	Int32 = 2,
	Long64 = 3,
	UTF8String = 5
};

enum TagPairType
{
	MainStream = 0,
	CalibrationStream = 1,
	SystemMetadata = 2,
	UserMetadata = 3
};

enum GetByteMode
{
	Normal = 0,
	KeyFrameBytes = 1,
	DiffCorrBytes = 2
};

enum DiffCorrBaseFrame
{
	DiffCorrKeyFrame = 0,
	DiffCorrPrevFrame = 1
};

enum ImageBytesLayout
{
	FullImageRaw = 0,
	FullImageDiffCorrWithSigns = 1
};

enum ImageByteOrder
{
	BigEndian = 0,
	LittleEndian = 1
};

namespace AdvLib2
{
	struct AdvFileInfo
	{
       int Width;
       int Height;
       int CountMaintFrames;
       int CountCalibrationFrames;
       int DataBpp;
       int MaxPixelValue;	   
       __int64 MainClockFrequency;
       int MainStreamAccuracy;
       __int64 CalibrationClockFrequency;
       int CalibrationStreamAccuracy;
	   unsigned char MainStreamTagsCount;
	   unsigned char CalibrationStreamTagsCount;
	   unsigned char SystemMetadataTagsCount;
	   unsigned char UserMetadataTagsCount;
	   __int64 UtcTimestampAccuracyInNanoseconds;
	   bool IsColourImage;
	   int ImageLayoutsCount;
	   int StatusTagsCount;
	};

	struct AdvFrameInfo
	{	
		unsigned int StartTicksLo;
		unsigned int StartTicksHi;
		unsigned int EndTicksLo;
		unsigned int EndTicksHi;
		
		unsigned int UtcTimestampLo;
		unsigned int UtcTimestampHi;
		unsigned int Exposure;

		float Gamma;
		float Gain;
		float Shutter;
		float Offset;

		unsigned char GPSTrackedSattelites;
		unsigned char GPSAlmanacStatus;
		unsigned char GPSFixStatus;
		char GPSAlmanacOffset;

		unsigned int VideoCameraFrameIdLo;
		unsigned int VideoCameraFrameIdHi;
		unsigned int HardwareTimerFrameIdLo;
		unsigned int HardwareTimerFrameIdHi;

		unsigned int SystemTimestampLo;
		unsigned int SystemTimestampHi;
	};
}

void crc32_init(void);
unsigned int compute_crc32(unsigned char *data, int len);

extern time_t TIME_ADV_ZERO;
extern tm* s_timeinfo;
extern time_t s_initTime;

#if __WIN32 || __WIN64
__int64 SystemTimeToAavTicks(SYSTEMTIME systemTime);
#endif

__int64 DateTimeToAavTicks(__int64 dayTicks, int hour, int minute, int sec, int tenthMs);
__int64 WindowsTicksToAavTicks(__int64 windowsTicks);
void DebugViewPrint(const wchar_t* formatText, ...);

#endif // UTILS_H