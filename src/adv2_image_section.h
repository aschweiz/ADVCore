/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_IMAGE_SECTION2_H
#define ADV_IMAGE_SECTION2_H

#include <stdio.h>
#include "adv2_image_layout.h"

namespace AdvLib2
{
	class Adv2ImageSection {

		public:
			unsigned int Width;
			unsigned int Height;
			unsigned char DataBpp;
		
		public:

			Adv2ImageSection(unsigned int width, unsigned int height, unsigned char dataBpp);
			~Adv2ImageSection();

			void WriteHeader(FILE* pfile);
			void BeginFrame();

			unsigned char* GetDataBytes(unsigned char layoutId, unsigned short* currFramePixels, unsigned int *bytesCount, char* byteMode, unsigned char pixelsBpp);
			AdvLib2::Adv2ImageLayout* GetImageLayoutById(unsigned char layoutId);

			int MaxFrameBufferSize();
	};
}

#endif //ADV_IMAGE_SECTION2_H