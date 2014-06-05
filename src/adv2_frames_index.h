/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_FRAME_INDEX2_H
#define ADV_FRAME_INDEX2_H

namespace AdvLib2
{
	class Adv2FramesIndex {

		public:
			Adv2FramesIndex();
			~Adv2FramesIndex();

			void WriteIndex(FILE *file);
			void AddFrame(unsigned char streamId, unsigned int frameNo, unsigned int elapedTime, __int64 frameOffset, unsigned int  bytesCount);
	};
}

#endif //ADV_FRAME_INDEX2_H