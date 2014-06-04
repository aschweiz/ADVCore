/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADVSTREAM2_H
#define ADVSTREAM2_H

#include <map>
#include <string>

using namespace std;
using std::string;

namespace AdvLib2
{
	class Adv2Stream {
		public:
			map<string, string> m_MetadataTags;

		private:
			__int64 m_ClockFrequency;
			unsigned int m_TimestampAccuracy;

			unsigned int m_FrameCount;

		public:
			Adv2Stream();
			~Adv2Stream();
	};
}

#endif //ADVSTREAM2_H