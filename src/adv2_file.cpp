/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "StdAfx.h"
#include "adv2_file.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include "utils.h"
#include "cross_platform.h"
#include "adv_profiling.h"

using namespace std;

namespace AdvLib2
{

FILE* m_Adv2File;
	
Adv2File::Adv2File()
{
	StatusSection = new AdvLib2::Adv2StatusSection();

	crc32_init();
	
	m_FrameBytes = NULL;
	ImageSection = NULL;
	m_Index = NULL;
}

Adv2File::~Adv2File()
{
	if (NULL != m_Adv2File)
	{
		advfclose(m_Adv2File);
		m_Adv2File = NULL;
	}
	
	if (NULL != ImageSection)
	{
		delete ImageSection;
		ImageSection = NULL;
	}
	
	if (NULL != StatusSection)
	{
		delete StatusSection;
		StatusSection = NULL;
	}
	
	if (NULL != m_Index)
	{
		delete m_Index;
		m_Index = NULL;
	}
	
	if (NULL != m_FrameBytes)
	{
		delete m_FrameBytes;
		m_FrameBytes = NULL;
	}

	m_UserMetadataTags.clear();
	m_FileTags.clear();
	m_MainStreamTags.clear();
	m_CalibrationStreamTags.clear();
}

unsigned char CURRENT_DATAFORMAT_VERSION = 2;

bool Adv2File::BeginFile(const char* fileName)
{
	m_Adv2File = advfopen(fileName, "wb");
	if (m_Adv2File == 0) return false;
	
	unsigned int buffInt;
	unsigned long buffLong;
	unsigned char buffChar;
	
	buffInt = 0x46545346;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfwrite(&CURRENT_DATAFORMAT_VERSION, 1, 1, m_Adv2File);

	buffInt = 0;
	buffLong = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // 0x00000000 (Reserved)
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of index table (will be saved later) 
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of system metadata table (will be saved later) 
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of user metadata table (will be saved later) 

	buffChar = (unsigned char)2;
	advfwrite(&buffChar, 1, 1, m_Adv2File); // Number of streams (main and calibration) 
	
	__int64 streamHeaderOffsetPositions[2];
	__int64 streamHeaderOffsets[2];

	WriteUTF8String(m_Adv2File, "MAIN");
	advfgetpos64(m_Adv2File, &m_MainFrameCountPosition);
	buffInt = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // Number of frames saved in the Main stream
	buffLong = m_MainStreamClockFrequency;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	buffInt = m_MainStreamTickAccuracy;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfgetpos64(m_Adv2File, &streamHeaderOffsetPositions[0]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later) 

	WriteUTF8String(m_Adv2File, "CALIBRATION");
	advfgetpos64(m_Adv2File, &m_CalibrationFrameCountPosition);
	buffInt = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // Number of frames saved in the Calibration stream
	buffLong = m_MainStreamClockFrequency;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	buffInt = m_MainStreamTickAccuracy;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfgetpos64(m_Adv2File, &streamHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later) 

	buffChar = (unsigned char)2;
	advfwrite(&buffChar, 1, 1, m_Adv2File); // Number of sections (image and status) 

	__int64 sectionHeaderOffsetPositions[2];
	
	WriteString(m_Adv2File, "IMAGE");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[0]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	
	WriteString(m_Adv2File, "STATUS");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);

	// Write section headers
	__int64 sectionHeaderOffsets[2];
	advfgetpos64(m_Adv2File, &sectionHeaderOffsets[0]);
	ImageSection->WriteHeader(m_Adv2File);
	advfgetpos64(m_Adv2File, &sectionHeaderOffsets[1]);
	StatusSection->WriteHeader(m_Adv2File);

	// Write section headers positions
	advfsetpos64(m_Adv2File, &sectionHeaderOffsetPositions[0]);
	advfwrite(&sectionHeaderOffsets[0], 8, 1, m_Adv2File);
	advfsetpos64(m_Adv2File, &sectionHeaderOffsetPositions[1]);
	advfwrite(&sectionHeaderOffsets[1], 8, 1, m_Adv2File);
	
	advfseek(m_Adv2File, 0, SEEK_END);
	
	// Write main stream metadata table
	unsigned int mainTagsCount = m_MainStreamTags.size();
	if (mainTagsCount > 0)
	{		
		advfgetpos64(m_Adv2File, &streamHeaderOffsets[0]);
	
		advfwrite(&mainTagsCount, 4, 1, m_Adv2File);
	
		map<string, string>::iterator curr = m_MainStreamTags.begin();
		while (curr != m_MainStreamTags.end()) 
		{
			char* tagName = const_cast<char*>(curr->first.c_str());
			WriteUTF8String(m_Adv2File, tagName);
		
			char* tagValue = const_cast<char*>(curr->second.c_str());
			WriteUTF8String(m_Adv2File, tagValue);
		
			curr++;
		}
	}

	// Write calibration stream metadata table
	unsigned int calibrationTagsCount = m_CalibrationStreamTags.size();
	if (calibrationTagsCount > 0)
	{		
		advfgetpos64(m_Adv2File, &streamHeaderOffsets[1]);
	
		advfwrite(&calibrationTagsCount, 4, 1, m_Adv2File);
	
		map<string, string>::iterator curr = m_CalibrationStreamTags.begin();
		while (curr != m_CalibrationStreamTags.end()) 
		{
			char* tagName = const_cast<char*>(curr->first.c_str());
			WriteUTF8String(m_Adv2File, tagName);
		
			char* tagValue = const_cast<char*>(curr->second.c_str());
			WriteUTF8String(m_Adv2File, tagValue);
		
			curr++;
		}
	}

	if (mainTagsCount > 0)
	{
		advfsetpos64(m_Adv2File, &streamHeaderOffsetPositions[0]);
		advfwrite(&streamHeaderOffsets[0], 8, 1, m_Adv2File);
	}

	if (calibrationTagsCount > 0)
	{
		advfsetpos64(m_Adv2File, &streamHeaderOffsetPositions[1]);
		advfwrite(&streamHeaderOffsets[1], 8, 1, m_Adv2File);
	}

	advfseek(m_Adv2File, 0, SEEK_END);
	
	// Write system metadata table
	__int64 systemMetadataTablePosition;
	advfgetpos64(m_Adv2File, &systemMetadataTablePosition);
	
	unsigned int fileTagsCount = m_FileTags.size();
	advfwrite(&fileTagsCount, 4, 1, m_Adv2File);
	
	map<string, string>::iterator curr = m_FileTags.begin();
	while (curr != m_FileTags.end()) 
	{
		char* tagName = const_cast<char*>(curr->first.c_str());	
		WriteString(m_Adv2File, tagName);
		
		char* tagValue = const_cast<char*>(curr->second.c_str());	
		WriteString(m_Adv2File, tagValue);
		
		curr++;
	}
	
	// Write system metadata table position to the file header
	advfseek(m_Adv2File, 0x11, SEEK_SET);
	advfwrite(&systemMetadataTablePosition, 8, 1, m_Adv2File);
	
	advfseek(m_Adv2File, 0, SEEK_END);
	
    m_Index = new AdvLib2::Adv2FramesIndex();
	
	advfflush(m_Adv2File);
		
	m_MainFrameNo = 0;
	m_CalibrationFrameNo = 0;

	m_UserMetadataTags.clear();

	return true;
}

bool Adv2File::LoadFile(const char* fileName)
{
	m_Adv2File = advfopen(fileName, "rb");
	if (m_Adv2File == 0) return false;
	
	unsigned int buffInt;
	unsigned long buffLong;
	unsigned char buffChar;
	
	unsigned char dataformatVersion;
	advfread(&buffInt, 4, 1, m_Adv2File);
	advfread(&dataformatVersion, 1, 1, m_Adv2File);

	if (buffInt != 0x46545346 || dataformatVersion != 2)
	{
		// Unsuported stream formar
		return 0;
	}

	advfread(&buffInt, 4, 1, m_Adv2File); // 0x00000000 (Reserved)

	__int64 systemMetadataTablePosition;
	__int64 indexTableOffset;		
	__int64 userMetaTableOffset;

	advfread(&indexTableOffset, 8, 1, m_Adv2File); // Offset of index table (will be saved later) 
	advfread(&systemMetadataTablePosition, 8, 1, m_Adv2File); // Offset of system metadata table (will be saved later) 
	advfread(&userMetaTableOffset, 8, 1, m_Adv2File); // Offset of user metadata table (will be saved later) 

	unsigned char numberOfStreams;
	advfread(&numberOfStreams, 1, 1, m_Adv2File); // Number of streams (must be 2: main and calibration) 

	__int64 streamHeaderOffsetPositions[2];
	__int64 streamHeaderOffsets[2];


	char* mainStreamName = ReadUTF8String(m_Adv2File);
	// TODO: CHECK Stream Name	
	delete mainStreamName;

	int numberOfMainFrames;
	advfread(&numberOfMainFrames, 4, 1, m_Adv2File); // Number of frames saved in the Main stream
	advfread(&m_MainStreamClockFrequency, 8, 1, m_Adv2File);
	advfread(&m_MainStreamTickAccuracy, 4, 1, m_Adv2File);
	advfread(&streamHeaderOffsetPositions[0], 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later)

	char* calibrationStreamName = ReadUTF8String(m_Adv2File);
	// TODO: CHECK Stream Name
	delete calibrationStreamName;

	// NOTE: These must be the same as the MAIN stream ones (or may be not?)
	__int64 calibrationStreamClockFrequency;
	int calibrationStreamTickAccuracy;

	int numberOfCalibrationFrames;
	advfread(&numberOfCalibrationFrames, 4, 1, m_Adv2File); // Number of frames saved in the Main stream
	advfread(&calibrationStreamClockFrequency, 8, 1, m_Adv2File);
	advfread(&calibrationStreamTickAccuracy, 4, 1, m_Adv2File);
	advfread(&streamHeaderOffsetPositions[1], 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later)

	/*

	
	advfgetpos64(m_Adv2File, &m_MainFrameCountPosition);
	buffInt = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // Number of frames saved in the Main stream

	buffLong = m_MainStreamClockFrequency;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	buffInt = m_MainStreamTickAccuracy;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfgetpos64(m_Adv2File, &streamHeaderOffsetPositions[0]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later) 

	WriteUTF8String(m_Adv2File, "CALIBRATION");
	advfgetpos64(m_Adv2File, &m_CalibrationFrameCountPosition);
	buffInt = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // Number of frames saved in the Calibration stream
	buffLong = m_MainStreamClockFrequency;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	buffInt = m_MainStreamTickAccuracy;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfgetpos64(m_Adv2File, &streamHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later) 

	buffChar = (unsigned char)2;
	advfwrite(&buffChar, 1, 1, m_Adv2File); // Number of sections (image and status) 

	__int64 sectionHeaderOffsetPositions[2];
	
	WriteString(m_Adv2File, "IMAGE");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[0]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	
	WriteString(m_Adv2File, "STATUS");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);

	// Write section headers
	__int64 sectionHeaderOffsets[2];
	advfgetpos64(m_Adv2File, &sectionHeaderOffsets[0]);
	ImageSection->WriteHeader(m_Adv2File);
	advfgetpos64(m_Adv2File, &sectionHeaderOffsets[1]);
	StatusSection->WriteHeader(m_Adv2File);

	// Write section headers positions
	advfsetpos64(m_Adv2File, &sectionHeaderOffsetPositions[0]);
	advfwrite(&sectionHeaderOffsets[0], 8, 1, m_Adv2File);
	advfsetpos64(m_Adv2File, &sectionHeaderOffsetPositions[1]);
	advfwrite(&sectionHeaderOffsets[1], 8, 1, m_Adv2File);
	
	advfseek(m_Adv2File, 0, SEEK_END);

	*/

	return true;
}

void Adv2File::SetTimingPrecision(__int64 mainClockFrequency, long mainStreamAccuracy, __int64 calibrationClockFrequency, long calibrationStreamAccuracy)
{
	m_MainStreamClockFrequency = mainClockFrequency;
	m_MainStreamTickAccuracy = mainStreamAccuracy;
	m_CalibrationStreamClockFrequency = calibrationClockFrequency;
	m_CalibrationStreamTickAccuracy = calibrationStreamAccuracy;
}

void Adv2File::EndFile()
{
	__int64 indexTableOffset;
	advfgetpos64(m_Adv2File, &indexTableOffset);
	
	m_Index->WriteIndex(m_Adv2File);
		
	__int64 userMetaTableOffset;
	advfgetpos64(m_Adv2File, &userMetaTableOffset);

	advfseek(m_Adv2File, m_MainFrameCountPosition, SEEK_SET);
	advfwrite(&m_MainFrameNo, 4, 1, m_Adv2File);

	advfseek(m_Adv2File, m_CalibrationFrameCountPosition, SEEK_SET);
	advfwrite(&m_CalibrationFrameNo, 4, 1, m_Adv2File);

	advfseek(m_Adv2File, 9, SEEK_SET);
	advfwrite(&indexTableOffset, 8, 1, m_Adv2File);
	advfseek(m_Adv2File, 0x19, SEEK_SET);	
	advfwrite(&userMetaTableOffset, 8, 1, m_Adv2File);
		
	// Write the metadata table
	advfseek(m_Adv2File, 0, SEEK_END);	

	unsigned int userTagsCount = m_UserMetadataTags.size();
	advfwrite(&userTagsCount, 4, 1, m_Adv2File);
	
	map<string, string>::iterator curr = m_UserMetadataTags.begin();
	while (curr != m_UserMetadataTags.end()) 
	{
		char* userTagName = const_cast<char*>(curr->first.c_str());	
		WriteString(m_Adv2File, userTagName);
		
		char* userTagValue = const_cast<char*>(curr->second.c_str());	
		WriteString(m_Adv2File, userTagValue);
		
		curr++;
	}
	
	
	advfflush(m_Adv2File);
	advfclose(m_Adv2File);	
	
	m_Adv2File = NULL;
}

void Adv2File::AddImageSection(AdvLib2::Adv2ImageSection* section)
{
	ImageSection = section;

	char convStr [10];
	snprintf(convStr, 10, "%d", section->Width);
	m_FileTags.insert(make_pair(string("WIDTH"), string(convStr)));
	
	snprintf(convStr, 10, "%d", section->Height);
	m_FileTags.insert(make_pair(string("HEIGHT"), string(convStr)));
	
	snprintf(convStr, 10, "%d", section->DataBpp);
	m_FileTags.insert(make_pair(string("BITPIX"), string(convStr)));
}

int Adv2File::AddMainStreamTag(const char* tagName, const char* tagValue)
{
	m_MainStreamTags.insert((make_pair(string(tagName == NULL ? "" : tagName), string(tagValue == NULL ? "" : tagValue))));
	
	return m_MainStreamTags.size();
}

int Adv2File::AddCalibrationStreamTag(const char* tagName, const char* tagValue)
{
	m_CalibrationStreamTags.insert((make_pair(string(tagName == NULL ? "" : tagName), string(tagValue == NULL ? "" : tagValue))));
	
	return m_CalibrationStreamTags.size();
}

int Adv2File::AddFileTag(const char* tagName, const char* tagValue)
{	
	m_FileTags.insert((make_pair(string(tagName == NULL ? "" : tagName), string(tagValue == NULL ? "" : tagValue))));
	
	return m_FileTags.size();
}

int Adv2File::AddUserTag(const char* tagName, const char* tagValue)
{
	m_UserMetadataTags.insert((make_pair(string(tagName == NULL ? "" : tagName), string(tagValue == NULL ? "" : tagValue))));
	
	return m_UserMetadataTags.size();	
}

void Adv2File::BeginFrame(unsigned char streamId, __int64 startFrameTicks, __int64 endFrameTicks,__int64 elapsedTicksSinceFirstFrame)
{
	AdvProfiling_StartBytesOperation();

	advfgetpos64(m_Adv2File, &m_NewFrameOffset);

	m_FrameBufferIndex = 0;
	m_CurrentStreamId = streamId;

	m_CurrentFrameElapsedTicks = elapsedTicksSinceFirstFrame;
		
	if (m_FrameBytes == NULL)
	{
		int maxUncompressedBufferSize = 
			4 + // frame start magic
			8 + // timestamp
			4 + // exposure			
			4 + 4 + // the length of each of the 2 sections 
			StatusSection->MaxFrameBufferSize +
			ImageSection->MaxFrameBufferSize() + 
			100; // Just in case
		
		m_FrameBytes = new unsigned char[maxUncompressedBufferSize];
	};		
	
	m_FrameBytes[0] = streamId;

	// Add the start timestamp
	m_FrameBytes[1] = (unsigned char)(startFrameTicks & 0xFF);
	m_FrameBytes[2] = (unsigned char)((startFrameTicks >> 8) & 0xFF);
	m_FrameBytes[3] = (unsigned char)((startFrameTicks >> 16) & 0xFF);
	m_FrameBytes[4] = (unsigned char)((startFrameTicks >> 24) & 0xFF);
	m_FrameBytes[5] = (unsigned char)((startFrameTicks >> 32) & 0xFF);
	m_FrameBytes[6] = (unsigned char)((startFrameTicks >> 40) & 0xFF);
	m_FrameBytes[7] = (unsigned char)((startFrameTicks >> 48) & 0xFF);
	m_FrameBytes[8] = (unsigned char)((startFrameTicks >> 56) & 0xFF);
	
	// Add the end timestamp
	m_FrameBytes[9] = (unsigned char)(endFrameTicks & 0xFF);
	m_FrameBytes[10] = (unsigned char)((endFrameTicks >> 8) & 0xFF);
	m_FrameBytes[11] = (unsigned char)((endFrameTicks >> 16) & 0xFF);
	m_FrameBytes[12] = (unsigned char)((endFrameTicks >> 24) & 0xFF);
	m_FrameBytes[13] = (unsigned char)((endFrameTicks >> 32) & 0xFF);
	m_FrameBytes[14] = (unsigned char)((endFrameTicks >> 40) & 0xFF);
	m_FrameBytes[15] = (unsigned char)((endFrameTicks >> 48) & 0xFF);
	m_FrameBytes[16] = (unsigned char)((endFrameTicks >> 56) & 0xFF);
	
	m_FrameBufferIndex = 17;
	
	StatusSection->BeginFrame();
	ImageSection->BeginFrame();	
	
	AdvProfiling_EndBytesOperation();
}

void Adv2File::AddFrameStatusTagUTF8String(unsigned int tagIndex, const char* tagValue)
{
	StatusSection->AddFrameStatusTagUTF8String(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagMessage(unsigned int tagIndex, const char* tagValue)
{
	StatusSection->AddFrameStatusTagMessage(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagUInt16(unsigned int tagIndex, unsigned short tagValue)
{
	StatusSection->AddFrameStatusTagUInt16(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagReal(unsigned int tagIndex, float tagValue)
{
	StatusSection->AddFrameStatusTagReal(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagUInt8(unsigned int tagIndex, unsigned char tagValue)
{
	StatusSection->AddFrameStatusTagUInt8(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagUInt32(unsigned int tagIndex, unsigned int tagValue)
{
	StatusSection->AddFrameStatusTagUInt32(tagIndex, tagValue);
}

void Adv2File::AddFrameStatusTagUInt64(unsigned int tagIndex, long long tagValue)
{
	StatusSection->AddFrameStatusTagUInt64(tagIndex, tagValue);
}

void Adv2File::AddFrameImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp)
{
	AdvProfiling_StartGenericProcessing();
	AdvProfiling_StartBytesOperation();
	
	unsigned int imageBytesCount = 0;	
	char byteMode = 0;
	m_CurrentImageLayout = ImageSection->GetImageLayoutById(layoutId);
	unsigned char *imageBytes = ImageSection->GetDataBytes(layoutId, pixels, &imageBytesCount, &byteMode, pixelsBpp);
	
	int imageSectionBytesCount = imageBytesCount + 2; // +1 byte for the layout id and +1 byte for the byteMode (See few lines below)
	
	m_FrameBytes[m_FrameBufferIndex] = imageSectionBytesCount & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 1] = (imageSectionBytesCount >> 8) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 2] = (imageSectionBytesCount >> 16) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 3] = (imageSectionBytesCount >> 24) & 0xFF;
	m_FrameBufferIndex+=4;
	
	// It is faster to write the layoutId and byteMode directly here
	m_FrameBytes[m_FrameBufferIndex] = m_CurrentImageLayout->LayoutId;
	m_FrameBytes[m_FrameBufferIndex + 1] = byteMode;
	m_FrameBufferIndex+=2;
		
	memcpy(&m_FrameBytes[m_FrameBufferIndex], &imageBytes[0], imageBytesCount);
	m_FrameBufferIndex+= imageBytesCount;
		
	unsigned int statusBytesCount = 0;
	unsigned char *statusBytes = StatusSection->GetDataBytes(&statusBytesCount);
	
	m_FrameBytes[m_FrameBufferIndex] = statusBytesCount & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 1] = (statusBytesCount >> 8) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 2] = (statusBytesCount >> 16) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 3] = (statusBytesCount >> 24) & 0xFF;
	m_FrameBufferIndex+=4;
	
	if (statusBytesCount > 0)
	{
		memcpy(&m_FrameBytes[m_FrameBufferIndex], &statusBytes[0], statusBytesCount);
		m_FrameBufferIndex+=statusBytesCount;

		delete statusBytes;
	}
	
	AdvProfiling_EndBytesOperation();
	AdvProfiling_EndGenericProcessing();
}
			
void Adv2File::EndFrame()
{	
	AdvProfiling_StartGenericProcessing();
	
	__int64 frameOffset;
	advfgetpos64(m_Adv2File, &frameOffset);
		
	// Frame start magic
	unsigned int frameStartMagic = 0xEE0122FF;
	advfwrite(&frameStartMagic, 4, 1, m_Adv2File);
	
	advfwrite(m_FrameBytes, m_FrameBufferIndex, 1, m_Adv2File);

	m_Index->AddFrame(m_CurrentStreamId, m_CurrentStreamId == 0 ? m_MainFrameNo : m_CalibrationFrameNo, m_CurrentFrameElapsedTicks, frameOffset, m_FrameBufferIndex);
	
	advfflush(m_Adv2File);
	
	if (m_CurrentStreamId == 0)
		m_MainFrameNo++;
	else
		m_CalibrationFrameNo++;

	AdvProfiling_NewFrameProcessed();
	
	AdvProfiling_EndGenericProcessing();
}

}
