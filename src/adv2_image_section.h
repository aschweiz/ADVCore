/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_IMAGE_SECTION2_H
#define ADV_IMAGE_SECTION2_H

#include <stdio.h>
#include "adv2_image_layout.h"
#include "utils.h"

#include <map>
#include <string>

using namespace std;
using std::string;

namespace AdvLib2
{
	enum GetByteOperation
	{
		None = 0,
		ConvertTo12BitPacked = 1,
		ConvertTo8BitBytesLooseHighByte = 2
	};

	class Adv2ImageLayout;

	class Adv2ImageSection {

	private:
		map<string, string> m_ImageTags;
		map<unsigned char, Adv2ImageLayout*> m_ImageLayouts;
		bool m_RGBorBGR;
	public:
		unsigned int Width;
		unsigned int Height;
		unsigned char DataBpp;
		
		enum ImageByteOrder ByteOrder;
		bool UsesCRC;
		int MaxPixelValue;
		bool IsColourImage;
		char ImageBayerPattern[128];

	public:

		Adv2ImageSection(unsigned int width, unsigned int height, unsigned char dataBpp);
		Adv2ImageSection(FILE* pfile, AdvFileInfo* fileInfo);
		~Adv2ImageSection();

		void WriteHeader(FILE* pfile);
		void BeginFrame();

		unsigned char* GetDataBytes(unsigned char layoutId, unsigned short* currFramePixels, unsigned int *bytesCount, unsigned char pixelsBpp, enum GetByteOperation operation);
		AdvLib2::Adv2ImageLayout* GetImageLayoutById(unsigned char layoutId);
		void AddOrUpdateTag(const char* tagName, const char* tagValue);
		Adv2ImageLayout* AddImageLayout(unsigned char layoutId, const char* layoutType, const char* compression, unsigned char layoutBpp);

		int MaxFrameBufferSize();

		void GetDataFromDataBytes(unsigned char* data, unsigned int* pixels, int sectionDataLength, int startOffset);
	};
}

#endif //ADV_IMAGE_SECTION2_H