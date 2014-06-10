/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_LIB
#define ADV_LIB

#include "adv_file.h"

extern char* g_CurrentAdvFile;
extern AdvLib::AdvFile* g_AdvFile;
extern bool g_FileStarted;

#ifdef __cplusplus
extern "C"
{
#endif 

DLL_PUBLIC char* AdvGetCurrentFilePath(void);

DLL_PUBLIC void AdvVer1_NewFile(const char* fileName);
DLL_PUBLIC void AdvVer1_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp);
DLL_PUBLIC void AdvVer1_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp, int keyFrame, const char* diffCorrFromBaseFrame);
DLL_PUBLIC unsigned int AdvVer1_DefineStatusSectionTag(const char* tagName, int tagType);
DLL_PUBLIC unsigned int AdvVer1_AddFileTag(const char* tagName, const char* tagValue);
DLL_PUBLIC void AdvVer1_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue);
DLL_PUBLIC void AdvVer1_EndFile();
DLL_PUBLIC bool AdvVer1_BeginFrame(long long timeStamp, unsigned int elapsedTime, unsigned int exposure);
DLL_PUBLIC void AdvVer1_FrameAddImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp);
DLL_PUBLIC void AdvVer1_FrameAddImageBytes(unsigned char layoutId, unsigned char* pixels, unsigned char pixelsBpp);
DLL_PUBLIC void AdvVer1_FrameAddStatusTag(unsigned int tagIndex, const char* tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTag32(unsigned int tagIndex, unsigned long tagValue);
DLL_PUBLIC void AdvVer1_FrameAddStatusTag64(unsigned int tagIndex, long long tagValue);
DLL_PUBLIC void AdvVer1_EndFrame();

DLL_PUBLIC void AdvVer2_NewFile(const char* fileName);
DLL_PUBLIC void AdvVer2_SetTimingPrecision(__int64 mainClockFrequency, long mainStreamAccuracy, __int64 calibrationClockFrequency, long calibrationStreamAccuracy);
DLL_PUBLIC unsigned int AdvVer2_AddMainStreamTag(const char* tagName, const char* tagValue);
DLL_PUBLIC unsigned int AdvVer2_AddCalibrationStreamTag(const char* tagName, const char* tagValue);
DLL_PUBLIC void AdvVer2_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp);
DLL_PUBLIC void AdvVer2_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp, int keyFrame, const char* diffCorrFromBaseFrame);
DLL_PUBLIC unsigned int AdvVer2_DefineStatusSectionTag(const char* tagName, int tagType);
DLL_PUBLIC unsigned int AdvVer2_AddFileTag(const char* tagName, const char* tagValue);
DLL_PUBLIC void AdvVer2_AddOrUpdateImageSectionTag(const char* tagName, const char* tagValue);
DLL_PUBLIC void AdvVer2_EndFile();
DLL_PUBLIC bool AdvVer2_BeginFrame(unsigned int streamId, long long timeStamp, long long elapsedTicks, unsigned int exposure);
DLL_PUBLIC void AdvVer2_FrameAddImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp);
DLL_PUBLIC void AdvVer2_FrameAddImageBytes(unsigned char layoutId, unsigned char* pixels, unsigned char pixelsBpp);
DLL_PUBLIC void AdvVer2_FrameAddStatusTagUTF8String(unsigned int tagIndex, const char* tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTagMessage(unsigned int tagIndex, const char* tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTag16(unsigned int tagIndex, unsigned short tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTagReal(unsigned int tagIndex, float tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTag32(unsigned int tagIndex, unsigned long tagValue);
DLL_PUBLIC void AdvVer2_FrameAddStatusTag64(unsigned int tagIndex, long long tagValue);
DLL_PUBLIC void AdvVer2_EndFrame();

//DLL_PUBLIC void AdvVer2_DefineImageSection(unsigned short width, unsigned short height, unsigned char dataBpp);
//DLL_PUBLIC void AdvVer2_DefineImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp, int keyFrame, const char* diffCorrFromBaseFrame);

DLL_PUBLIC void GetLibraryVersion(char* version);
DLL_PUBLIC void GetLibraryPlatformId(char* platform);

#ifdef __cplusplus
}
#endif
 
#endif

