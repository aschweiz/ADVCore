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
	m_TagDefinition.insert(make_pair(string(tagName), (AdvTagType)tagType));
	
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

float IntToFloat(unsigned int x)
{
	union 
	{
		float f;
		unsigned int i;
	} u;

	u.i = x;
	return u.f;
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
		
		map<string, AdvTagType>::iterator currDef = m_TagDefinition.find(tagName);

		buffChar = (unsigned char)(int)((AdvTagType)(currDef->second));
		advfwrite(&buffChar, 1, 1, pFile);		
		
		m_TagDefinitionNames.pop_front();
	}
	m_TagDefinition.empty();
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

void Adv2StatusSection::GetDataFromDataBytes(unsigned char* data, int sectionDataLength, int startOffset, AdvFrameInfo* frameInfo, char* systemError)
{
	unsigned char* statusData = data + startOffset;
	unsigned char tagsCount = *statusData;
	statusData++;

	for(int i = 0; i < tagsCount; i++)
	{
		unsigned char tagId = *statusData;
		
		string currById = m_FrameStatusTags[tagId];
		const char* tagName = currById.c_str();
		map<string, AdvTagType>::iterator currDef = m_TagDefinition.find(tagName);
		AdvTagType type = (AdvTagType)(currDef->second);
		
		if (strcmp("GPSTrackedSatellites", tagName) == 0)
		{
			char val = *(statusData + 1);
			frameInfo->GPSTrackedSattelites = val;
			statusData+=2;
		}
		else if (strcmp("GPSAlmanacStatus", tagName) == 0)
		{
			char val = *(statusData + 1);
			frameInfo->GPSAlmanacStatus = val;
			statusData+=2;
		}
		else if (strcmp("GPSAlmanacOffset", tagName) == 0)
		{
			char val = *(statusData + 1);
			frameInfo->GPSAlmanacOffset = val;
			statusData+=2;
		}
		else if (strcmp("GPSFixStatus", tagName) == 0)
		{
			char val = *(statusData + 1);
			frameInfo->GPSFixStatus = val;
			statusData+=2;
		}
		else if (strcmp("Gain", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);

			unsigned int value = (unsigned int)(((int)b4 << 24) + ((int)b3 << 16) + ((int)b2 << 8) + (int)b1);
			float fVal = IntToFloat(value);

			frameInfo->Gain = fVal;
			
			statusData+=5;
		}
		else if (strcmp("Gamma", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);

			unsigned int value = (unsigned int)(((int)b4 << 24) + ((int)b3 << 16) + ((int)b2 << 8) + (int)b1);
			float fVal = IntToFloat(value);

			frameInfo->Gamma = fVal;
			
			statusData+=5;			
		}
		else if (strcmp("Temperature", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);

			unsigned int value = (unsigned int)(((int)b4 << 24) + ((int)b3 << 16) + ((int)b2 << 8) + (int)b1);
			float fVal = IntToFloat(value);

			frameInfo->Temperature = fVal;
			
			statusData+=5;			
		}
		else if (strcmp("Shutter", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);

			unsigned int value = (unsigned int)(((int)b4 << 24) + ((int)b3 << 16) + ((int)b2 << 8) + (int)b1);
			float fVal = IntToFloat(value);

			frameInfo->Shutter = fVal;

			statusData+=5;			
		}
		else if (strcmp("Offset", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);

			unsigned int value = (unsigned int)(((int)b4 << 24) + ((int)b3 << 16) + ((int)b2 << 8) + (int)b1);
			float fVal = IntToFloat(value);

			frameInfo->Offset = fVal;
			
			statusData+=5;			
		}
		else if (strcmp("VideoCameraFrameId", tagName) == 0)
		{
			unsigned char  b1 = *(statusData + 1);
			unsigned char  b2 = *(statusData + 2);
			unsigned char  b3 = *(statusData + 3);
			unsigned char  b4 = *(statusData + 4);
			unsigned char  b5 = *(statusData + 5);
			unsigned char  b6 = *(statusData + 6);
			unsigned char  b7 = *(statusData + 7);
			unsigned char  b8 = *(statusData + 8);

			long valLo = (long)(((long)b4 << 24) + ((long)b3 << 16) + ((long)b2 << 8) + (long)b1);
			long valHi = (long)(((long)b8 << 24) + ((long)b7 << 16) + ((long)b6 << 8) + (long)b5);

			if (strcmp("VideoCameraFrameId", tagName) == 0)
			{
				frameInfo->VideoCameraFrameIdLo = valLo;
				frameInfo->VideoCameraFrameIdHi = valHi;			
			}
			
			statusData+=9;
		}
		else if (
			strcmp("SystemError", tagName) == 0)
		{
			char* destBuffer = systemError;

			unsigned char count = *(statusData + 1);
			statusData += 2;
			for (int j = 0; j < count; j++)
			{
				unsigned char len = *statusData;
				
				if (destBuffer != NULL)
				{
					strncpy(destBuffer,  (char*)(statusData + 1), len);
					destBuffer+=len;
					*destBuffer = '\n';
					*(destBuffer + 1) = '\r';
					destBuffer+=2;

					*destBuffer = '\0';
				}

				statusData += 1 + len;
			}			
		}
		else
		{
			switch(type)
			{
				case UInt8:
					statusData+=2;
					break;
				case UInt16:
					statusData+=3;
					break;
				case UInt32:
					statusData+=5;
					break;
				case ULong64:
					statusData+=9;
					break;
				case AnsiString255:
					{
						unsigned char strLen = *(statusData + 1);
						statusData += 2 + strLen;
					}
					break;
				case List16OfAnsiString255:
					{
						unsigned char count = *(statusData + 1);
						statusData += 2;
						for (int j = 0; j < count; j++)
						{
							unsigned char len = *statusData;
							statusData += 1 + len;
						}							
					}
					break;
			}
		}
	}
}



}