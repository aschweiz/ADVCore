/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_image_layout.h"
#include "utils.h"

namespace AdvLib2
{

Adv2ImageLayout::Adv2ImageLayout(unsigned int width, unsigned int height, unsigned char layoutId, const char* layoutType, const char* compression, unsigned char bpp, int keyFrame)
{
	// TODO:
}

Adv2ImageLayout::~Adv2ImageLayout()
{

}

void Adv2ImageLayout::AddOrUpdateTag(const char* tagName, const char* tagValue)
{
	map<string, string>::iterator curr = m_LayoutTags.begin();
	while (curr != m_LayoutTags.end()) 
	{
		const char* existingTagName = curr->first.c_str();
		
		if (0 == strcmp(existingTagName, tagName))
		{
			m_LayoutTags.erase(curr);
			break;
		}
		
		curr++;
	}
	
	m_LayoutTags.insert(make_pair(string(tagName), string(tagValue == NULL ? "" : tagValue)));

	if (0 == strcmp("DIFFCODE-BASE-FRAME", tagName))
	{
		if (0 == strcmp("KEY-FRAME", tagValue))
		{
			//BaseFrameType = DiffCorrKeyFrame;
		}
		else if (0 == strcmp("PREV-FRAME", tagValue))
		{
			//BaseFrameType = DiffCorrPrevFrame;
		}
	}
	
	if (0 == strcmp("DATA-LAYOUT", tagName))
	{
		//m_BytesLayout = FullImageRaw;
		//if (0 == strcmp("FULL-IMAGE-DIFFERENTIAL-CODING", tagValue)) m_BytesLayout = FullImageDiffCorrWithSigns;
		//IsDiffCorrLayout = m_BytesLayout == FullImageDiffCorrWithSigns;
	}
}

void Adv2ImageLayout::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 2;
	fwrite(&buffChar, 1, 1, pFile); /* Version */

	fwrite(&Bpp, 1, 1, pFile);	

	
	buffChar = (unsigned char)m_LayoutTags.size();
	fwrite(&buffChar, 1, 1, pFile);
	
	map<string, string>::iterator curr = m_LayoutTags.begin();
	while (curr != m_LayoutTags.end()) 
	{
		char* tagName = const_cast<char*>(curr->first.c_str());	
		WriteUTF8String(pFile, tagName);
		
		char* tagValue = const_cast<char*>(curr->second.c_str());	
		WriteUTF8String(pFile, tagValue);
		
		curr++;
	}		
}

}