/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_IMAGE_LAYOUT2_H
#define ADV_IMAGE_LAYOUT2_H

#include <map>
#include <string>

#include "utils.h"

#include "adv2_image_section.h"
#include "cross_platform.h"

#include "quicklz.h"
#include "Compressor.h"

using namespace std;
using std::string;

namespace AdvLib2
{
	class Adv2ImageSection;

	class Adv2ImageLayout {

	private:
		Adv2ImageSection* m_ImageSection;
		unsigned char *SIGNS_MASK;
		map<string, string> m_LayoutTags;			
		ImageBytesLayout m_BytesLayout;

		int m_KeyFrameBytesCount;
		unsigned short *m_PrevFramePixels;
		unsigned short *m_PrevFramePixelsTemp;
		unsigned char *m_PixelArrayBuffer;
		unsigned char *m_SignsBuffer;
		unsigned int m_MaxSignsBytesCount;
		unsigned int m_MaxPixelArrayLengthWithoutSigns;

		char* m_DecompressedPixels;
		qlz_state_decompress* m_StateDecompress;
		Compressor* m_Lagarith16Decompressor;
		bool m_UsesCompression;

		void InitialiseBuffers();
		void ResetBuffers();

	public:
		unsigned char LayoutId;
		unsigned int Width;
		unsigned int Height;
		unsigned char Bpp;
		unsigned char DataBpp;
	
		const char* Compression;
		bool IsDiffCorrLayout;
		int KeyFrame;
		
		int MaxFrameBufferSize;

		enum DiffCorrBaseFrame BaseFrameType;

	public:
			Adv2ImageLayout(Adv2ImageSection* imageSection, unsigned int width, unsigned int height, unsigned char layoutId, const char* layoutType, const char* compression, unsigned char dataBpp, int keyFrame);
			~Adv2ImageLayout();

			void AddOrUpdateTag(const char* tagName, const char* tagValue);
			void WriteHeader(FILE* pfile);
	};
}

#endif //ADV_IMAGE_LAYOUT2_H