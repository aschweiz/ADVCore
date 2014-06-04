/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_image_section.h"

namespace AdvLib2
{

Adv2ImageSection::Adv2ImageSection(unsigned int width, unsigned int height, unsigned char dataBpp)
{

}

Adv2ImageSection::~Adv2ImageSection()
{

}

void Adv2ImageSection::WriteHeader(FILE* pfile)
{

}

void Adv2ImageSection::BeginFrame()
{

}

int Adv2ImageSection::MaxFrameBufferSize()
{

}

unsigned char* Adv2ImageSection::GetDataBytes(unsigned char layoutId, unsigned short* currFramePixels, unsigned int *bytesCount, char* byteMode, unsigned char pixelsBpp)
{
}

AdvLib2::Adv2ImageLayout* Adv2ImageSection::GetImageLayoutById(unsigned char layoutId)
{

}


}