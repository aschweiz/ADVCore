/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_frames_index.h"

namespace AdvLib2
{

Adv2FramesIndex::Adv2FramesIndex()
{
	m_MainIndexEntries = new vector<Index2Entry*>();
	m_CalibrationIndexEntries = new vector<Index2Entry*>();
}

Adv2FramesIndex::~Adv2FramesIndex()
{
	m_MainIndexEntries->clear();
	delete m_MainIndexEntries;

	m_CalibrationIndexEntries->clear();
	delete m_CalibrationIndexEntries;
}

void Adv2FramesIndex::WriteIndex(FILE *file)
{
	unsigned char buffInt8 = 2;
	fwrite(&buffInt8, 1, 1, file);

	unsigned int buffOffset = 9;
	fwrite(&buffOffset, 4, 1, file);

	buffOffset = m_MainIndexEntries->size() * 20 + 10;
	fwrite(&buffOffset, 4, 1, file);

	unsigned int framesCount = m_MainIndexEntries->size();
	fwrite(&framesCount, 4, 1, file);

	vector<Index2Entry*>::iterator curr = m_MainIndexEntries->begin();
	while (curr != m_MainIndexEntries->end()) 
	{
		__int64 elapedTime = (*curr)->ElapsedTicks;
		__int64 frameOffset = (*curr)->FrameOffset;
		unsigned int  bytesCount = (*curr)->BytesCount;
		
		fwrite(&elapedTime, 8, 1, file);
		fwrite(&frameOffset, 8, 1, file);
		fwrite(&bytesCount, 4, 1, file);
		
		curr++;
	}


	framesCount = m_CalibrationIndexEntries->size();
	fwrite(&framesCount, 4, 1, file);

	curr = m_CalibrationIndexEntries->begin();
	while (curr != m_CalibrationIndexEntries->end()) 
	{
		__int64 elapedTime = (*curr)->ElapsedTicks;
		__int64 frameOffset = (*curr)->FrameOffset;
		unsigned int  bytesCount = (*curr)->BytesCount;
		
		fwrite(&elapedTime, 8, 1, file);
		fwrite(&frameOffset, 8, 1, file);
		fwrite(&bytesCount, 4, 1, file);
		
		curr++;
	}
}

void Adv2FramesIndex::AddFrame(unsigned char streamId, unsigned int frameNo, __int64 elapsedTicks, __int64 frameOffset, unsigned int  bytesCount)
{
	Index2Entry *entry = new Index2Entry();
	entry->BytesCount = bytesCount;
	entry->FrameOffset = frameOffset;
	entry->ElapsedTicks = elapsedTicks;
	
	if (streamId == 0)
		m_MainIndexEntries->push_back(entry);
	else
		m_CalibrationIndexEntries->push_back(entry);
}

}