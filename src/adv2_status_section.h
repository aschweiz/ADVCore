/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_STATUS_SECTION2_H
#define ADV_STATUS_SECTION2_H

#include <map>
#include <list>
#include <vector>
#include <string>
#include <stdio.h>
#include "utils.h"

using namespace std;
using std::string;

namespace AdvLib2
{
	class Adv2StatusSection {

		private:
			vector<string> m_TagDefinitionNames;
			map<string, Adv2TagType> m_TagDefinition;
		
			map<unsigned int, string> m_FrameStatusTags;
			map<unsigned int, unsigned char> m_FrameStatusTagsUInt8;
			map<unsigned int, unsigned short> m_FrameStatusTagsUInt16;
			map<unsigned int, unsigned int> m_FrameStatusTagsUInt32;
			map<unsigned int, __int64> m_FrameStatusTagsUInt64;
			map<unsigned int, float> m_FrameStatusTagsReal;

			__int64 m_UtcStartTimeNanosecondsSinceAdvZeroEpoch;
			unsigned int m_UtcExposureNanoseconds;

		public:
			int MaxFrameBufferSize;
			__int64 UtcTimestampAccuracyInNanoseconds;

		public:
			Adv2StatusSection(__int64 utcTimestampAccuracyInNanoseconds);
			Adv2StatusSection(FILE* pFile, AdvFileInfo* fileInfo);
			~Adv2StatusSection();

			unsigned int DefineTag(const char* tagName, enum Adv2TagType tagType);

			void BeginFrame(__int64 utcStartTimeNanosecondsSinceAdvZeroEpoch, unsigned int utcExposureNanoseconds);
			void WriteHeader(FILE* pfile);
			void AddFrameStatusTagUTF8String(unsigned int tagIndex, const char* tagValue);
			void AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue);
			void AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue);
			void AddFrameStatusTagReal(unsigned int tagIndex, float tagValue);
			void AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue);
			void AddFrameStatusTagUInt64(unsigned int tagIndex, __int64 tagValue);

			unsigned char* GetDataBytes(unsigned int *bytesCount);
			void GetDataFromDataBytes(unsigned char* data, int sectionDataLength, int startOffset, AdvFrameInfo* frameInfo, int* systemErrorLen);

			HRESULT GetStatusTagNameSize(int tagId, int* tagNameSize);
			HRESULT GetStatusTagInfo(int tagId, char* tagName, Adv2TagType* tagType);
			HRESULT GetStatusTagSizeUTF8String(unsigned int tagIndex, int* tagValueSize);
			HRESULT GetStatusTagUTF8String(unsigned int tagIndex, char* tagValue);
			HRESULT GetStatusTagUInt8(unsigned int tagIndex, unsigned char* tagValue);
			HRESULT GetStatusTag16(unsigned int tagIndex, unsigned short* tagValue);
			HRESULT GetStatusTagReal(unsigned int tagIndex, float* tagValue);
			HRESULT GetStatusTag32(unsigned int tagIndex, unsigned int* tagValue);
			HRESULT GetStatusTag64(unsigned int tagIndex, __int64* tagValue);
	};
}

#endif //ADV_STATUS_SECTION2_H