/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_STATUS_SECTION2_H
#define ADV_STATUS_SECTION2_H

#include <stdio.h>

namespace AdvLib2
{
	class Adv2StatusSection {

		public:
			int MaxFrameBufferSize;

		public:
			Adv2StatusSection();
			~Adv2StatusSection();

			void BeginFrame();
			void WriteHeader(FILE* pfile);
			void AddFrameStatusTag(unsigned int tagIndex, const char* tagValue);
			void AddFrameStatusTagMessage(unsigned int tagIndex, const char* tagValue);
			void AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue);
			void AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue);
			void AddFrameStatusTagReal(unsigned int tagIndex, float tagValue);
			void AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue);
			void AddFrameStatusTagUInt64(unsigned int tagIndex, long long tagValue);
			unsigned char* GetDataBytes(unsigned int *bytesCount);
	};
}

#endif //ADV_STATUS_SECTION2_H