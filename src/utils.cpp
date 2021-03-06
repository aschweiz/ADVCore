/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "utils.h"
#include <string>
#include <stdlib.h>

void WriteString(FILE* pFile, const char* str)
{
	unsigned char len;
	len = static_cast<char>(strlen(str));
	
	advfwrite(&len, 1, 1, pFile);
	advfwrite(&str[0], len, 1, pFile);
}

void WriteUTF8String(FILE* pFile, const char* str)
{
	unsigned short len;
	len = static_cast<char>(strlen(str));
	
	advfwrite(&len, 2, 1, pFile);
	advfwrite(&str[0], len, 1, pFile);
}

char* ReadString(FILE* pFile)
{
	unsigned char len;
	
	advfread(&len, 1, 1, pFile);
	char* str = (char*)malloc(len + 1);
	advfread(&str[0], len, 1, pFile);
	*(str + len) = 0;
	return str;
}

char* ReadUTF8String(FILE* pFile)
{
	unsigned short len;
	
	advfread(&len, 2, 1, pFile);
	char* str = (char*)malloc(len + 1);
	advfread(&str[0], len, 1, pFile);
	*(str + len) = 0;
	return str;
}

unsigned int crctab[256];

void crc32_init(void)
{
    int i,j;

    unsigned int crc;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04c11db7;
            else
                crc = crc << 1;
        }
        crctab[i] = crc;
    }
}

unsigned int compute_crc32(unsigned char *data, int len)
{
    unsigned int        result;
    int                 i;

    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~ result;
    len -=4;
    
    for (i=0; i<len; i++)
    {
        result = (result << 8 | *data++) ^ crctab[result >> 24];
    }
    
    return ~result;
}

#define ADV_EPOCH_ZERO_TICKS 633979008000000000
#define EPOCH_1601_JAN_1_TICKS 504911232000000000

#if __WIN32 || __WIN64
__int64 SystemTimeToAavTicks(SYSTEMTIME systemTime)
{
	FILETIME fileTime;
	SystemTimeToFileTime(&systemTime, &fileTime);

	ULARGE_INTEGER uli;
	uli.LowPart = fileTime.dwLowDateTime;
	uli.HighPart = fileTime.dwHighDateTime;

	return WindowsTicksToAavTicks(uli.QuadPart + EPOCH_1601_JAN_1_TICKS);
}
#endif

__int64 DateTimeToAavTicks(__int64 dayTicks, int hour, int minute, int sec, int tenthMs)
{
	if (dayTicks > 0)
	{
		__int64 advTicks = 
				(__int64)(dayTicks - ADV_EPOCH_ZERO_TICKS) / 10000 + 
				(__int64)(hour * 3600 + minute * 60 + sec) * 1000 +
				tenthMs / 10;

		return advTicks;		
	}
	else
		return 0;
}

__int64 WindowsTicksToAavTicks(__int64 windowsTicks)
{
	if (windowsTicks > 0)
	{
		__int64 advTicks = 
				(__int64)(windowsTicks - ADV_EPOCH_ZERO_TICKS) / 10000;

		return advTicks;		
	}
	else
		return 0;
}

void DebugViewPrint(const wchar_t* formatText, ...)
{
#ifdef MSVC	
	wchar_t debug512CharBuffer[512];
    va_list args;
    va_start(args, formatText);
	vswprintf(debug512CharBuffer, 512, formatText, args);
    
	OutputDebugString(debug512CharBuffer);
	va_end(args);
#endif	
}