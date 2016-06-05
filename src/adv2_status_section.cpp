/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "adv2_status_section.h"
#include <string>
#include <stdlib.h>

using namespace std;
using std::string;

namespace AdvLib2
{

Adv2StatusSection::Adv2StatusSection()
{
	MaxFrameBufferSize = 0;
}

Adv2StatusSection::~Adv2StatusSection()
{

}

unsigned int Adv2StatusSection::DefineTag(const char* tagName, AdvTagType tagType)
{
	m_TagDefinitionNames.push_back(string(tagName));
	m_TagDefinitionTypes.push_back(tagType);
	
	switch(tagType)
	{
		case UInt8:
			MaxFrameBufferSize+=1;
			break;
			
		case UInt16:
			MaxFrameBufferSize+=2;
			break;

		case UInt32:
			MaxFrameBufferSize+=4;
			break;
			
		case ULong64:
			MaxFrameBufferSize+=8;
			break;			
			
		case Real:
			MaxFrameBufferSize+=4;
			break;	
			
		case UTF8String:
			MaxFrameBufferSize+=0x10001;
			break;
			
		case List16OfUTF8String:
			MaxFrameBufferSize+=16 * 0x10001;
			break;
	}
	
	return (unsigned int)m_TagDefinitionNames.size() - 1;
}


void Adv2StatusSection::BeginFrame()
{
	m_FrameStatusTags.clear();
	m_FrameStatusTagsUInt8.clear();
	m_FrameStatusTagsUInt16.clear();
	m_FrameStatusTagsUInt64.clear();
	m_FrameStatusTagsUInt32.clear();
	m_FrameStatusTagsReal.clear();
	
	m_FrameStatusTagsMessages.clear();
}

void Adv2StatusSection::AddFrameStatusTagUTF8String(unsigned int tagIndex, const char* tagValue)
{
	m_FrameStatusTags.insert(make_pair(tagIndex, string(tagValue == nullptr ? "" : tagValue)));
}

void Adv2StatusSection::AddFrameStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{
		list<string> messageList = m_FrameStatusTagsMessages[tagIndex];
	
	if (messageList.size() == 16) messageList.pop_front();
	
	messageList.push_back(string(tagValue == nullptr ? "" : tagValue));
	
	m_FrameStatusTagsMessages[tagIndex] = messageList;
}

void Adv2StatusSection::AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	m_FrameStatusTagsUInt8.insert(make_pair(tagIndex, tagValue));
}

void Adv2StatusSection::AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue)
{
	m_FrameStatusTagsUInt16.insert(make_pair(tagIndex, tagValue));
}

void Adv2StatusSection::AddFrameStatusTagReal(unsigned int tagIndex, float tagValue)
{
	m_FrameStatusTagsReal.insert(make_pair(tagIndex, tagValue));
}

void Adv2StatusSection::AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue)
{
	m_FrameStatusTagsUInt32.insert(make_pair(tagIndex, tagValue));
}

void Adv2StatusSection::AddFrameStatusTagUInt64(unsigned int tagIndex, __int64 tagValue)
{
	m_FrameStatusTagsUInt64.insert(make_pair(tagIndex, tagValue));
}


unsigned int FloatToIntBits(const float x)
{
    union {
       float f;  		  // assuming 32-bit IEEE 754 single-precision
       unsigned int i;    // assuming 32-bit 2's complement int
    } u;
        
    u.f = x;
    return u.i;
}

Adv2StatusSection::Adv2StatusSection(FILE* pFile)
{
	MaxFrameBufferSize = 0;

	unsigned char version;
	advfread(&version, 1, 1, pFile); /* Version */

	unsigned char tagsCount;
	advfread(&tagsCount, 1, 1, pFile);

	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(pFile);
		unsigned char tagType;
		advfread(&tagType, 1, 1, pFile);

		DefineTag(tagName, (AdvTagType)tagType);
	}
}

void Adv2StatusSection::WriteHeader(FILE* pFile)
{
	unsigned char buffChar;
	
	buffChar = 1;
	advfwrite(&buffChar, 1, 1, pFile); /* Version */
	
	buffChar = (unsigned char)m_TagDefinitionNames.size();
	advfwrite(&buffChar, 1, 1, pFile);
	int tagCount = buffChar;
	
	for(int i = 0; i<tagCount; i++)
	{
		char* tagName = const_cast<char*>(m_TagDefinitionNames.front().c_str());
		WriteUTF8String(pFile, tagName);
		
		buffChar = (unsigned char)(int)(m_TagDefinitionTypes.front());
		advfwrite(&buffChar, 1, 1, pFile);		
		
		m_TagDefinitionNames.pop_front();
		m_TagDefinitionTypes.pop_front();
	}
}

unsigned char* Adv2StatusSection::GetDataBytes(unsigned int *bytesCount)
{
	int size = 0;
	int arrayLength = 0;
	int numTagEntries = 0;
	
	map<unsigned int, string>::iterator curr = m_FrameStatusTags.begin();
	while (curr != m_FrameStatusTags.end()) 
	{
		char* tagValue = const_cast<char*>(curr->second.c_str());
		
		arrayLength += (int)strlen(tagValue) + 1 /* TagId*/  + 2 /* length */ ;
		curr++;
		numTagEntries++;
	}
	
	map<unsigned int, list<string> >::iterator currLst = m_FrameStatusTagsMessages.begin();
	while (currLst != m_FrameStatusTagsMessages.end()) 
	{
		list<string> lst = currLst->second;
		list<string>::iterator currMsg = lst.begin();
		while (currMsg != lst.end()) 
		{
			char* tagValue = const_cast<char*>(currMsg->c_str());
			
			arrayLength += (int)strlen(tagValue) + 2 /* length*/;
			currMsg++;
		}
		arrayLength+= (1 /* TagId*/ + 1 /* num messages*/);
		currLst++;
		numTagEntries++;
	}
	
	arrayLength += (int)m_FrameStatusTagsUInt8.size() * (1 /*sizeof(unsigned char)*/ + 1 /* TagId*/ );
	numTagEntries += (int)m_FrameStatusTagsUInt8.size();
	arrayLength += (int)m_FrameStatusTagsUInt16.size() * (2 /*sizeof(unsigned short)*/ + 1 /* TagId*/ );
	numTagEntries += (int)m_FrameStatusTagsUInt16.size();
	arrayLength += (int)m_FrameStatusTagsUInt64.size() * (8 /*sizeof(__int64)*/ + 1 /* TagId*/ );
	numTagEntries += (int)m_FrameStatusTagsUInt64.size();
	arrayLength += (int)m_FrameStatusTagsUInt32.size() * (4 /*sizeof(unsinged int)*/ + 1 /* TagId*/ );
	numTagEntries += (int)m_FrameStatusTagsUInt32.size();
	arrayLength += (int)m_FrameStatusTagsReal.size() * (4 /*sizeof(float)*/ + 1 /* TagId*/ );
	numTagEntries += (int)m_FrameStatusTagsReal.size();
	
	size = arrayLength + 1;
	
	unsigned char *statusData = (unsigned char*)malloc(size);
	statusData[0] = (numTagEntries & 0xFF);		
		
	if (arrayLength > 0)
	{
		int dataPos = 1;
		
		map<unsigned int, __int64>::iterator currUInt64 = m_FrameStatusTagsUInt64.begin();
		while (currUInt64 != m_FrameStatusTagsUInt64.end()) 
		{
			unsigned char tagId = (unsigned char)(currUInt64->first & 0xFF);
			statusData[dataPos] = tagId;

			__int64 tagValue = (__int64)(currUInt64->second);
			statusData[dataPos + 1] = (unsigned char)(tagValue & 0xFF);
			statusData[dataPos + 2] = (unsigned char)((tagValue >> 8) & 0xFF);
			statusData[dataPos + 3] = (unsigned char)((tagValue >> 16) & 0xFF);
			statusData[dataPos + 4] = (unsigned char)((tagValue >> 24) & 0xFF);
			statusData[dataPos + 5] = (unsigned char)((tagValue >> 32) & 0xFF);
			statusData[dataPos + 6] = (unsigned char)((tagValue >> 40) & 0xFF);
			statusData[dataPos + 7] = (unsigned char)((tagValue >> 48) & 0xFF);
			statusData[dataPos + 8] = (unsigned char)((tagValue >> 56) & 0xFF);
	
			dataPos+=9;
			
			currUInt64++;
		}

		map<unsigned int, unsigned int>::iterator currUInt32 = m_FrameStatusTagsUInt32.begin();
		while (currUInt32 != m_FrameStatusTagsUInt32.end()) 
		{
			unsigned char tagId = (unsigned char)(currUInt32->first & 0xFF);
			statusData[dataPos] = tagId;

			unsigned int tagValue = (__int64)(currUInt32->second);
			statusData[dataPos + 1] = (unsigned char)(tagValue & 0xFF);
			statusData[dataPos + 2] = (unsigned char)((tagValue >> 8) & 0xFF);
			statusData[dataPos + 3] = (unsigned char)((tagValue >> 16) & 0xFF);
			statusData[dataPos + 4] = (unsigned char)((tagValue >> 24) & 0xFF);
	
			dataPos+=5;
			
			currUInt32++;
		}
		
		map<unsigned int, unsigned short>::iterator currUInt16 = m_FrameStatusTagsUInt16.begin();
		while (currUInt16 != m_FrameStatusTagsUInt16.end()) 
		{
			unsigned char tagId = (unsigned char)(currUInt16->first & 0xFF);
			statusData[dataPos] = tagId;

			unsigned short tagValue = (unsigned short)(currUInt16->second);
			statusData[dataPos + 1] = (unsigned char)(tagValue & 0xFF);
			statusData[dataPos + 2] = (unsigned char)((tagValue >> 8) & 0xFF);

			dataPos+=3;
			
			currUInt16++;
		}
		
		map<unsigned int, unsigned char>::iterator currUInt8 = m_FrameStatusTagsUInt8.begin();
		while (currUInt8 != m_FrameStatusTagsUInt8.end()) 
		{
			unsigned char tagId = (unsigned char)(currUInt8->first & 0xFF);
			statusData[dataPos] = tagId;

			unsigned char tagValue = (unsigned char)(currUInt8->second);
			statusData[dataPos + 1] = tagValue;

			dataPos+=2;
			
			currUInt8++;
		}
		
		map<unsigned int, float>::iterator currReal = m_FrameStatusTagsReal.begin();
		while (currReal != m_FrameStatusTagsReal.end()) 
		{
			unsigned char tagId = (unsigned char)(currReal->first & 0xFF);
			statusData[dataPos] = tagId;

			float tagValue = (float)(currReal->second);
			unsigned int intValue = FloatToIntBits(tagValue);
			
			statusData[dataPos + 1] = intValue & 0xFF;
			statusData[dataPos + 2] = (intValue >> 8) & 0xFF;
			statusData[dataPos + 3] = (intValue >> 16) & 0xFF;
			statusData[dataPos + 4] = (intValue >> 24) & 0xFF;

			dataPos+=5;
			
			currReal++;
		}
		
		curr = m_FrameStatusTags.begin();
		while (curr != m_FrameStatusTags.end()) 
		{
			unsigned char tagId = (unsigned char)(curr->first & 0xFF);
			statusData[dataPos] = tagId;
			
			char* tagValue = const_cast<char*>(curr->second.c_str());
			
			int strLen = (int)strlen(tagValue);
			statusData[dataPos + 1] = strLen & 0xFF;
			statusData[dataPos + 2] = (strLen >> 8) & 0xFF;
			memcpy(&statusData[dataPos + 3], tagValue, strLen);
			dataPos+= strLen + 3;
			
			curr++;
		}

		currLst = m_FrameStatusTagsMessages.begin();
		while (currLst != m_FrameStatusTagsMessages.end()) 
		{
			unsigned char tagId = (unsigned char)(currLst->first & 0xFF);
			statusData[dataPos] = tagId;
			
			list<string> lst = currLst->second;
			statusData[dataPos + 1] = (unsigned char)lst.size();
			
			dataPos+=2;
			
			list<string>::iterator currMsg = lst.begin();
			while (currMsg != lst.end()) 
			{
				char* tagValue = const_cast<char*>(currMsg->c_str());
				
				int strLen = (int)strlen(tagValue);
				statusData[dataPos] = strLen & 0xFF;
				statusData[dataPos + 1] = (strLen >> 8) & 0xFF;
				memcpy(&statusData[dataPos + 2], tagValue, strLen);
				dataPos+= strLen + 2;
			
				currMsg++;
			}

			currLst++;
		}	
	}
	
	*bytesCount = size;
	return statusData;
}

}