/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
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
	crc32_init();
	
	m_FrameBytes = nullptr;
	ImageSection = nullptr;
	m_Index = nullptr;

	m_NumberOfMainFrames = 0;
	m_NumberOfCalibrationFrames = 0;
	m_UsesExternalMainStreamClock = false;
	m_UsesExternalCalibrationStreamClock = false;
	TotalNumberOfMainFrames = 0;
	TotalNumberOfCalibrationFrames = 0;
}

Adv2File::~Adv2File()
{
	CloseFile();
}

unsigned char CURRENT_DATAFORMAT_VERSION = 2;

bool Adv2File::BeginFile(const char* fileName)
{
	m_Adv2File = advfopen(fileName, "wb");
	if (m_Adv2File == 0) return false;
	
	unsigned int buffInt;
	__int64 buffLong;
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

	__int64 internalFrequency = advgetclockresolution();
	if (!m_UsesExternalMainStreamClock)
	{
		m_MainStreamClockFrequency = internalFrequency;
		m_MainStreamTickAccuracy = 0; // Unknown accuracy as it is 'automatically' timestamped at frame save time
	}

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

	if (!m_UsesExternalCalibrationStreamClock)
	{
		m_CalibrationStreamClockFrequency = internalFrequency;
		m_CalibrationStreamTickAccuracy = 0; // Unknown accuracy as it is 'automatically' timestamped at frame save time
	}

	WriteUTF8String(m_Adv2File, "CALIBRATION");
	advfgetpos64(m_Adv2File, &m_CalibrationFrameCountPosition);
	buffInt = 0;
	advfwrite(&buffInt, 4, 1, m_Adv2File); // Number of frames saved in the Calibration stream
	buffLong = m_CalibrationStreamClockFrequency;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	buffInt = m_CalibrationStreamTickAccuracy;
	advfwrite(&buffInt, 4, 1, m_Adv2File);
	advfgetpos64(m_Adv2File, &streamHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later) 

	buffChar = (unsigned char)2;
	advfwrite(&buffChar, 1, 1, m_Adv2File); // Number of sections (image and status) 

	__int64 sectionHeaderOffsetPositions[2];
	
	WriteUTF8String(m_Adv2File, "IMAGE");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[0]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	
	WriteUTF8String(m_Adv2File, "STATUS");
	advfgetpos64(m_Adv2File, &sectionHeaderOffsetPositions[1]);
	buffLong = 0;
	advfwrite(&buffLong, 8, 1, m_Adv2File);
	
	// Write main stream metadata table
	unsigned int mainTagsCount = (unsigned int)m_MainStreamTags.size();
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
	unsigned int calibrationTagsCount = (unsigned int)m_CalibrationStreamTags.size();
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

	// Write system metadata table
	__int64 systemMetadataTablePosition;
	advfgetpos64(m_Adv2File, &systemMetadataTablePosition);
	
	unsigned int fileTagsCount = (unsigned int)m_FileTags.size();
	advfwrite(&fileTagsCount, 4, 1, m_Adv2File);
	
	map<string, string>::iterator curr = m_FileTags.begin();
	while (curr != m_FileTags.end()) 
	{
		char* tagName = const_cast<char*>(curr->first.c_str());	
		WriteUTF8String(m_Adv2File, tagName);
		
		char* tagValue = const_cast<char*>(curr->second.c_str());	
		WriteUTF8String(m_Adv2File, tagValue);
		
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

    advfflush(m_Adv2File);
    
	return true;
}

int Adv2File::LoadFile(const char* fileName, AdvFileInfo* fileInfo)
{
	TotalNumberOfMainFrames = 0;
	TotalNumberOfCalibrationFrames = 0;

	m_Adv2File = advfopen(fileName, "rb");
	if (m_Adv2File == 0) return false;
	
	unsigned int buffInt;
	
	unsigned char dataformatVersion;
	advfread(&buffInt, 4, 1, m_Adv2File);
	advfread(&dataformatVersion, 1, 1, m_Adv2File);

	if (buffInt != 0x46545346 || dataformatVersion != 2)
	{
		// Unsuported stream format
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

	__int64 streamHeaderOffsets[2];

	char* mainStreamName = ReadUTF8String(m_Adv2File);
	if (strcmp(mainStreamName, "MAIN") != 0)
	{
		delete mainStreamName;
		return -1;
	}
	delete mainStreamName;

	advfread(&m_NumberOfMainFrames, 4, 1, m_Adv2File); // Number of frames saved in the Main stream
	advfread(&m_MainStreamClockFrequency, 8, 1, m_Adv2File);
	advfread(&m_MainStreamTickAccuracy, 4, 1, m_Adv2File);
	advfread(&streamHeaderOffsets[0], 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later)

	fileInfo->MainClockFrequency = m_MainStreamClockFrequency;
	fileInfo->MainStreamAccuracy = m_MainStreamTickAccuracy;
	fileInfo->CountMaintFrames = m_NumberOfMainFrames;

	TotalNumberOfMainFrames = m_NumberOfMainFrames;	

	char* calibrationStreamName = ReadUTF8String(m_Adv2File);
	if (strcmp(calibrationStreamName, "CALIBRATION") != 0)
	{
		delete calibrationStreamName;
		return -1;
	}
	delete calibrationStreamName;

	advfread(&m_NumberOfCalibrationFrames, 4, 1, m_Adv2File); // Number of frames saved in the Calibration stream
	advfread(&m_CalibrationStreamClockFrequency, 8, 1, m_Adv2File);
	advfread(&m_CalibrationStreamTickAccuracy, 4, 1, m_Adv2File);
	advfread(&streamHeaderOffsets[1], 8, 1, m_Adv2File); // Offset of main stream metadata table (will be saved later)

	fileInfo->CalibrationClockFrequency = m_CalibrationStreamClockFrequency;
	fileInfo->CalibrationStreamAccuracy = m_CalibrationStreamTickAccuracy;
	fileInfo->CountCalibrationFrames = m_NumberOfCalibrationFrames;

	TotalNumberOfCalibrationFrames = m_NumberOfCalibrationFrames;

	unsigned char numberSections;
	advfread(&numberSections, 1, 1, m_Adv2File); // Number of sections (image and status) 
	if (numberSections != 2)
	{
		return -2;
	}

	__int64 sectionHeaderOffsets[2];

	char* imageSectionName = ReadUTF8String(m_Adv2File);
	if (strcmp(imageSectionName, "IMAGE") != 0)
	{
		delete imageSectionName;
		return -2;
	}
	delete imageSectionName;
	advfread(&sectionHeaderOffsets[0], 8, 1, m_Adv2File);

	char* statusSectionName = ReadUTF8String(m_Adv2File);
	if (strcmp(statusSectionName, "STATUS") != 0)
	{
		delete statusSectionName;
		return -2;
	}
	delete statusSectionName;
	advfread(&sectionHeaderOffsets[1], 8, 1, m_Adv2File);

	advfsetpos64(m_Adv2File, &sectionHeaderOffsets[0]);
	ImageSection = new AdvLib2::Adv2ImageSection(m_Adv2File);

	fileInfo->Width = ImageSection->Width;
	fileInfo->Height = ImageSection->Height;
	fileInfo->DataBpp = ImageSection->DataBpp;
	fileInfo->MaxPixelValue = ImageSection->MaxPixelValue;
	fileInfo->IsColourImage = ImageSection->IsColourImage;

	advfsetpos64(m_Adv2File, &sectionHeaderOffsets[1]);
	StatusSection = new AdvLib2::Adv2StatusSection(m_Adv2File);

	fileInfo->UtcTimestampAccuracyInNanoseconds = StatusSection->UtcTimestampAccuracyInNanoseconds;

	unsigned char tagsCount;

	// Read MAIN stream metadata table
	advfsetpos64(m_Adv2File, &streamHeaderOffsets[0]);
	advfread(&tagsCount, 1, 1, m_Adv2File);
	fileInfo->MainStreamTagsCount = tagsCount;
	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(m_Adv2File);
		char* tagValue = ReadUTF8String(m_Adv2File);

		m_MainStreamTags.insert(make_pair(tagName, tagValue));
	}

	// Read CALIBRATION stream metadata table
	advfsetpos64(m_Adv2File, &streamHeaderOffsets[1]);
	advfread(&tagsCount, 1, 1, m_Adv2File);
	fileInfo->CalibrationStreamTagsCount = tagsCount;
	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(m_Adv2File);
		char* tagValue = ReadUTF8String(m_Adv2File);

		m_CalibrationStreamTags.insert(make_pair(tagName, tagValue));
	}

	// Read system metadata table
	advfsetpos64(m_Adv2File, &systemMetadataTablePosition);
	advfread(&tagsCount, 1, 1, m_Adv2File);
	fileInfo->SystemMetadataTagsCount = tagsCount;
	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(m_Adv2File);
		char* tagValue = ReadUTF8String(m_Adv2File);

		m_FileTags.insert(make_pair(tagName, tagValue));
	}

	advfsetpos64(m_Adv2File, &indexTableOffset);
	m_Index = new AdvLib2::Adv2FramesIndex(m_Adv2File);

	advfsetpos64(m_Adv2File, &userMetaTableOffset);
	advfread(&tagsCount, 1, 1, m_Adv2File);
	fileInfo->UserMetadataTagsCount = tagsCount;
	for (int i = 0; i < tagsCount; i++)
	{
		char* tagName = ReadUTF8String(m_Adv2File);
		char* tagValue = ReadUTF8String(m_Adv2File);

		m_UserMetadataTags.insert(make_pair(tagName, tagValue));
	}

	return true;
}

bool Adv2File::CloseFile()
{
	bool fileClosed = false;
	if (nullptr != m_Adv2File)
	{
		advfclose(m_Adv2File);
		m_Adv2File = nullptr;
		fileClosed = true;
	}
	
	if (nullptr != ImageSection)
	{
		delete ImageSection;
		ImageSection = nullptr;
	}
	
	if (nullptr != StatusSection)
	{
		delete StatusSection;
		StatusSection = nullptr;
	}
	
	if (nullptr != m_Index)
	{
		delete m_Index;
		m_Index = nullptr;
	}
	
	if (nullptr != m_FrameBytes)
	{
		delete m_FrameBytes;
		m_FrameBytes = nullptr;
	}

	m_UserMetadataTags.clear();
	m_FileTags.clear();
	m_MainStreamTags.clear();
	m_CalibrationStreamTags.clear();

	return fileClosed;
}

void Adv2File::SetTicksTimingPrecision(int mainStreamAccuracy, int calibrationStreamAccuracy)
{
	m_MainStreamTickAccuracy = mainStreamAccuracy;
	m_CalibrationStreamTickAccuracy = calibrationStreamAccuracy;
}

void Adv2File::DefineExternalClockForMainStream(__int64 clockFrequency, int ticksTimingAccuracy)
{
	m_UsesExternalMainStreamClock = true;
	m_MainStreamClockFrequency = clockFrequency;
	m_MainStreamTickAccuracy = ticksTimingAccuracy;
}

void Adv2File::DefineExternalClockForCalibrationStream(__int64 clockFrequency, int ticksTimingAccuracy)
{
	m_UsesExternalCalibrationStreamClock = true;
	m_CalibrationStreamClockFrequency = clockFrequency;
	m_CalibrationStreamTickAccuracy = ticksTimingAccuracy;
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

	unsigned int userTagsCount = (unsigned int)m_UserMetadataTags.size();
	advfwrite(&userTagsCount, 4, 1, m_Adv2File);
	
	map<string, string>::iterator curr = m_UserMetadataTags.begin();
	while (curr != m_UserMetadataTags.end()) 
	{
		char* userTagName = const_cast<char*>(curr->first.c_str());	
		WriteUTF8String(m_Adv2File, userTagName);
		
		char* userTagValue = const_cast<char*>(curr->second.c_str());	
		WriteUTF8String(m_Adv2File, userTagValue);
		
		curr++;
	}
	
	
	advfflush(m_Adv2File);
	advfclose(m_Adv2File);
	
	m_Adv2File = nullptr;
}

void Adv2File::AddImageSection(AdvLib2::Adv2ImageSection* section)
{
	ImageSection = section;

	char convStr [10];
	_snprintf_s(convStr, 10, "%d", section->Width);
	m_FileTags.insert(make_pair(string("WIDTH"), string(convStr)));
	
	_snprintf_s(convStr, 10, "%d", section->Height);
	m_FileTags.insert(make_pair(string("HEIGHT"), string(convStr)));
	
	_snprintf_s(convStr, 10, "%d", section->DataBpp);
	m_FileTags.insert(make_pair(string("BITPIX"), string(convStr)));
}

void Adv2File::AddStatusSection(AdvLib2::Adv2StatusSection* section)
{
	StatusSection = section;
}

int Adv2File::AddMainStreamTag(const char* tagName, const char* tagValue)
{
	m_MainStreamTags.insert((make_pair(string(tagName == nullptr ? "" : tagName), string(tagValue == nullptr ? "" : tagValue))));
	
	return (int)m_MainStreamTags.size();
}

int Adv2File::AddCalibrationStreamTag(const char* tagName, const char* tagValue)
{
	m_CalibrationStreamTags.insert((make_pair(string(tagName == nullptr ? "" : tagName), string(tagValue == nullptr ? "" : tagValue))));
	
	return (int)m_CalibrationStreamTags.size();
}

int Adv2File::AddFileTag(const char* tagName, const char* tagValue)
{	
	m_FileTags.insert((make_pair(string(tagName == nullptr ? "" : tagName), string(tagValue == nullptr ? "" : tagValue))));
	
	return (int)m_FileTags.size();
}

int Adv2File::AddUserTag(const char* tagName, const char* tagValue)
{
	m_UserMetadataTags.insert((make_pair(string(tagName == nullptr ? "" : tagName), string(tagValue == nullptr ? "" : tagValue))));
	
	return (int)m_UserMetadataTags.size();	
}

void Adv2File::BeginFrame(unsigned char streamId, __int64 utcStartTimeNanosecondsSinceAdvZeroEpoch, unsigned int utcExposureNanoseconds)
{
	__int64 endFrameTicks = advgetclockticks();

	if (m_Index->GetFramesCount(streamId) == 0)
	{
		// First frame in stream
		m_FirstFrameInStreamTicks[streamId] = endFrameTicks;
		m_PrevFrameInStreamTicks[streamId] = endFrameTicks;
	}

	__int64 startFrameTicks = m_PrevFrameInStreamTicks[streamId];
	m_PrevFrameInStreamTicks[streamId] = endFrameTicks;
	__int64 elapsedTicksSinceFirstFrame = endFrameTicks - m_FirstFrameInStreamTicks[streamId];

	BeginFrame(streamId, startFrameTicks, endFrameTicks, elapsedTicksSinceFirstFrame, utcStartTimeNanosecondsSinceAdvZeroEpoch, utcExposureNanoseconds);
}

void Adv2File::BeginFrame(unsigned char streamId, __int64 startFrameTicks, __int64 endFrameTicks,__int64 elapsedTicksSinceFirstFrame, __int64 utcStartTimeNanosecondsSinceAdvZeroEpoch, unsigned int utcExposureNanoseconds)
{
	AdvProfiling_StartBytesOperation();

	advfgetpos64(m_Adv2File, &m_NewFrameOffset);

	m_FrameBufferIndex = 0;
	m_CurrentStreamId = streamId;

	m_CurrentFrameElapsedTicks = elapsedTicksSinceFirstFrame;
		
	if (m_FrameBytes == nullptr)
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
	
	ImageSection->BeginFrame();	
	StatusSection->BeginFrame(utcStartTimeNanosecondsSinceAdvZeroEpoch, utcExposureNanoseconds);

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

void Adv2File::AddFrameStatusTagUInt64(unsigned int tagIndex, __int64 tagValue)
{
	StatusSection->AddFrameStatusTagUInt64(tagIndex, tagValue);
}

/* Assumed pixel format by AdvCore when this method is called

    |    Layout Type    |  ImageSection.DataBpp |          Assumed Pixel Format                                 |
	|-------------------|-----------------------|---------------------------------------------------------------|
    |  FULL-IMAGE-RAW   |    16, 12             | 16-bit input (1 short per pixel)                              |
	|  FULL-IMAGE-RAW   |    8                  | 16-bit input will be converted to 1 byte per pixel            |
    |12BIT-IMAGE-PACKED |    12                 | 16-bit input (1 short per pixel) will be packed when storing  |
    
	All other combinations which are not listed above are invalid.
*/
HRESULT Adv2File::AddFrameImage(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp)
{
	unsigned char bpp = ImageSection->DataBpp;
	m_CurrentImageLayout = ImageSection->GetImageLayoutById(layoutId);

	if (m_CurrentImageLayout->Is12BitImagePacked && bpp == 12)
	{
		AddFrameImageInternal(layoutId, pixels, pixelsBpp, GetByteOperation::ConvertTo12BitPacked);
		return S_OK;
	}
	else if (m_CurrentImageLayout->IsFullImageRaw && bpp == 8)
	{
		AddFrameImageInternal(layoutId, pixels, pixelsBpp, GetByteOperation::ConvertTo8BitBytesLooseHighByte);
		return S_OK;
	}
	else if (m_CurrentImageLayout->IsFullImageRaw)
	{
		AddFrameImageInternal(layoutId, pixels, pixelsBpp, GetByteOperation::None);
		return S_OK;
	}

	return E_FAIL;
}

/* Assumed pixel format by AdvCore when this method is called

    |    Layout Type    |  ImageSection.DataBpp |  Assumed Pixel Format                                         |
	|-------------------|-----------------------|---------------------------------------------------------------|
    |  FULL-IMAGE-RAW   |        16, 12         | 16-bit little endian data passed as bytes (2 bytes per pixel) |
	|  FULL-IMAGE-RAW   |         8             | 8-bit data passed as bytes (1 byte per pixel)                 |
    |12BIT-IMAGE-PACKED |        12             | 12-bit packed data (3 bytes per 2 pixels)                     |
    | 8BIT-COLOR-IMAGE  |         8             | 8-bit RGB or BGR data (3 bytes per pixel, 1 colour per byte)  |

	All other combinations which are not listed above are invalid.
*/
HRESULT Adv2File::AddFrameImage(unsigned char layoutId, unsigned char* pixels, unsigned char pixelsBpp)
{
	unsigned char bpp = ImageSection->DataBpp;
	m_CurrentImageLayout = ImageSection->GetImageLayoutById(layoutId);

	if (m_CurrentImageLayout->Is12BitImagePacked && bpp == 12)
	{
		AddFrameImageInternal(layoutId, (unsigned short*)pixels, pixelsBpp, GetByteOperation::None);
		return S_OK;
	}
	else if (m_CurrentImageLayout->IsFullImageRaw)
	{
		AddFrameImageInternal(layoutId, (unsigned short*)pixels, pixelsBpp, GetByteOperation::None);
		return S_OK;
	}
	else if (m_CurrentImageLayout->Is8BitColourImage && bpp == 8)
	{
		return E_NOTIMPL;
	}

	return E_FAIL;
}

void Adv2File::AddFrameImageInternal(unsigned char layoutId, unsigned short* pixels, unsigned char pixelsBpp, enum GetByteOperation operation)
{
	AdvProfiling_StartGenericProcessing();
	AdvProfiling_StartBytesOperation();
	
	unsigned int imageBytesCount = 0;	
	unsigned char *imageBytes = ImageSection->GetDataBytes(layoutId, pixels, &imageBytesCount, pixelsBpp, operation);
	
	int imageSectionBytesCount = imageBytesCount + 2; // +1 byte for the layout id and +1 byte for the byteMode (See few lines below)
	
	m_FrameBytes[m_FrameBufferIndex] = imageSectionBytesCount & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 1] = (imageSectionBytesCount >> 8) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 2] = (imageSectionBytesCount >> 16) & 0xFF;
	m_FrameBytes[m_FrameBufferIndex + 3] = (imageSectionBytesCount >> 24) & 0xFF;
	m_FrameBufferIndex+=4;
	
	// It is faster to write the layoutId and byteMode directly here
	m_FrameBytes[m_FrameBufferIndex] = layoutId;
	m_FrameBytes[m_FrameBufferIndex + 1] = 0; // byteMode of Normal (reserved for future use of differential coding)
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

void Adv2File::GetFrameImageSectionHeader(int streamId, int frameId, unsigned char* layoutId, enum GetByteMode* mode)
{
	AdvLib2::Index2Entry* indexEntry = m_Index->GetIndexForFrame(streamId, frameId);

	advfsetpos64(m_Adv2File, &indexEntry->FrameOffset);


	long frameDataMagic;
	fread(&frameDataMagic, 4, 1, m_Adv2File);

	if (frameDataMagic == 0xEE0122FF)
	{
		// Skip 1 byte of streamId, 16 bytes of start and end timestamps and 4 bytes of section size
		fseek(m_Adv2File, 1 + 16 + 4, SEEK_CUR);

		fread(layoutId, 1, 1, m_Adv2File);

		unsigned char byteMode;

		fread(&byteMode, 1, 1, m_Adv2File);

		*mode = (GetByteMode)byteMode;
	}
}

void Adv2File::GetFrameSectionData(int streamId, int frameId, unsigned int* pixels, AdvFrameInfo* frameInfo, char* systemError)
{
	AdvLib2::Index2Entry* indexEntry = m_Index->GetIndexForFrame(streamId, frameId);

	advfsetpos64(m_Adv2File, &indexEntry->FrameOffset);

	long frameDataMagic;
	fread(&frameDataMagic, 4, 1, m_Adv2File);

	if (frameDataMagic == 0xEE0122FF)
	{
		unsigned char* data = (unsigned char*)malloc(indexEntry->BytesCount);
		fread(data, indexEntry->BytesCount, 1, m_Adv2File);

		// data[0] is the streamId

		// Read the timestamp and exposure 
		frameInfo->StartTicksLo = data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24);
		frameInfo->StartTicksHi = data[5] + (data[6] << 8) + (data[7] << 16) + (data[8] << 24);
		frameInfo->EndTicksLo = data[9] + (data[10] << 8) + (data[11] << 16) + (data[12] << 24);
		frameInfo->EndTicksHi = data[13] + (data[14] << 8) + (data[15] << 16) + (data[16] << 24);

	    int dataOffset = 17;
		int sectionDataLength = data[dataOffset] + (data[dataOffset + 1] << 8) + (data[dataOffset + 2] << 16) + (data[dataOffset + 3] << 24);

		ImageSection->GetDataFromDataBytes(data, pixels, sectionDataLength, dataOffset + 4);
		dataOffset += sectionDataLength + 4;

		sectionDataLength = data[dataOffset] + (data[dataOffset + 1] << 8) + (data[dataOffset + 2] << 16) + (data[dataOffset + 3] << 24);
		StatusSection->GetDataFromDataBytes(data, sectionDataLength, dataOffset + 4, frameInfo, systemError);
		
		delete data;
	}
}

}
