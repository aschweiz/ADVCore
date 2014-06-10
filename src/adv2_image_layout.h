/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_IMAGE_LAYOUT2_H
#define ADV_IMAGE_LAYOUT2_H

#include <map>
#include <string>

using namespace std;
using std::string;

namespace AdvLib2
{
	class Adv2ImageLayout {

	private:
		map<string, string> m_LayoutTags;

	public:
		unsigned char LayoutId;
		unsigned int Width;
		unsigned int Height;
		unsigned char Bpp;

		int MaxFrameBufferSize;

		public:
			Adv2ImageLayout(unsigned int width, unsigned int height, unsigned char layoutId, const char* layoutType, const char* compression, unsigned char bpp, int keyFrame);
			~Adv2ImageLayout();

			void AddOrUpdateTag(const char* tagName, const char* tagValue);
			void WriteHeader(FILE* pfile);
	};
}

#endif //ADV_IMAGE_LAYOUT2_H