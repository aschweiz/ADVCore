/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef ADV_ERROR_CODES
#define ADV_ERROR_CODES


typedef int ADVRESULT;

#define E_ADV_NOFILE ((ADVRESULT)0x81000001L)

#define E_ADV_STATUS_ENTRY_ALREADY_ADDED ((ADVRESULT)0x81001001L)
#define E_ADV_INVALID_STATUS_TAG_ID ((ADVRESULT)0x81001002L)
#define E_ADV_INVALID_STATUS_TAG_TYPE ((ADVRESULT)0x81001003L)

#ifndef _WIN32
#define S_OK 0
#define E_FAIL ((ADVRESULT)0x80004005L)
#define E_NOTIMPL ((ADVRESULT)0x80004001L)
#endif

#endif