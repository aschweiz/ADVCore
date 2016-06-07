/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADVFILE2_H
#define ADVFILE2_H

#include "adv2_image_section.h"
#include "adv2_status_section.h"
#include "adv2_image_layout.h"
#include "adv2_frames_index.h"

#include <map>
#include <string>
#include <stdio.h>
#include "quicklz.h"

using namespace std;
using std::string;

namespace AdvLib2
{

	class Adv2File {
		public:
			AdvLib2::Adv2ImageSection* ImageSection;
			AdvLib2::Adv2StatusSection* StatusSection;

		protected:
			AdvLib2::Adv2FramesIndex* m_Index;
			map<string, string> m_FileTags;
			
		private:
			AdvLib2::Adv2ImageLayout* m_CurrentImageLayout;
			unsigned char m_CurrentStreamId;

			__int64 m_NewFrameOffset;

			__int64 m_MainFrameCountPosition;
			__int64 m_CalibrationFrameCountPosition;

			unsigned int m_MainFrameNo;
			unsigned int m_CalibrationFrameNo;

			unsigned char *m_FrameBytes;
			unsigned int m_FrameBufferIndex; 
			__int64 m_CurrentFrameElapsedTicks;

			map<string, string> m_UserMetadataTags;

			map<string, string> m_MainStreamTags;
			map<string, string> m_CalibrationStreamTags;

			__int64 m_MainStreamClockFrequency;
			unsigned int m_MainStreamTickAccuracy;
			__int64 m_CalibrationStreamClockFrequency;
			unsigned int m_CalibrationStreamTickAccuracy;
			bool m_UsesCustomMainStreamClock;
			bool m_UsesCustomCalibrationStreamClock;

			int m_NumberOfMainFrames;
			int m_NumberOfCalibrationFrames;

			void InitFileState();
		public:
			int TotalNumberOfMainFrames;
			int TotalNumberOfCalibrationFrames;

			Adv2File();
			~Adv2File();
			
			bool BeginFile(const char* fileName);
			void SetTicksTimingPrecision(int mainStreamAccuracy, int calibrationStreamAccuracy);
			void DefineCustomClockForMainStream(__int64 clockFrequency, int ticksTimingAccuracy);
			void DefineCustomClockForCalibrationStream(__int64 clockFrequency, int ticksTimingAccuracy);
			void EndFile();
			
			int LoadFile(const char* fileName);
			bool CloseFile();
			
			void AddImageSection(AdvLib2::Adv2ImageSection* section);
			
			int AddFileTag(const char* tagName, const char* tagValue);
			int AddUserTag(const char* tagName, const char* tagValue);

			int AddMainStreamTag(const char* tagName, const char* tagValue);
			int AddCalibrationStreamTag(const char* tagName, const char* tagValue);
			
			void BeginFrame(unsigned char streamId, __int64 startFrameTicks, __int64 endFrameTicks,__int64 elapsedTicksSinceFirstFrame);
			void BeginFrame(unsigned char streamId);
			void EndFrame();

			void AddFrameStatusTagUTF8String(unsigned int tagIndex, const char* tagValue);
			void AddFrameStatusTagMessage(unsigned int tagIndex, const char* tagValue);
			void AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue);
			void AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue);
			void AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue);
			void AddFrameStatusTagUInt64(unsigned int tagIndex, __int64 tagValue);
			void AddFrameStatusTagReal(unsigned int tagIndex, float tagValue);
			void AddFrameImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp);

			void GetMainStreamInfo(int* numFrames, __int64* mainClockFrequency, int* mainStreamAccuracy);
			void GetCalibrationStreamInfo(int* numFrames, __int64* calibrationClockFrequency, int* calibrationStreamAccuracy);

			void GetFrameImageSectionHeader(int streamId, int frameId, unsigned char* layoutId, enum GetByteMode* mode);
			void GetFrameSectionData(int streamId, int frameId, unsigned int* prevFrame, unsigned int* pixels, AdvFrameInfo* frameInfo, char* systemError);
		};

}

#endif // ADVFILE2_H
