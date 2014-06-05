/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_status_section.h"

namespace AdvLib2
{

Adv2StatusSection::Adv2StatusSection()
{

}

Adv2StatusSection::~Adv2StatusSection()
{

}

void Adv2StatusSection::BeginFrame()
{
}

void Adv2StatusSection::WriteHeader(FILE* pfile)
{
}

void Adv2StatusSection::AddFrameStatusTag(unsigned int tagIndex, const char* tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagReal(unsigned int tagIndex, float tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue)
{
}

void Adv2StatusSection::AddFrameStatusTagUInt64(unsigned int tagIndex, long long tagValue)
{
}

unsigned char* Adv2StatusSection::GetDataBytes(unsigned int *bytesCount)
{
	return 0;
}

}