/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_frames_index.h"

namespace AdvLib2
{

Adv2FramesIndex::Adv2FramesIndex()
{

}

Adv2FramesIndex::~Adv2FramesIndex()
{

}

void Adv2FramesIndex::WriteIndex(FILE *file)
{

}

void Adv2FramesIndex::AddFrame(unsigned char streamId, unsigned int frameNo, unsigned int elapedTime, __int64 frameOffset, unsigned int  bytesCount)
{
	// TODO: Add the right index entry for the right stream 
}

}