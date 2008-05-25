#pragma once

#define INITGUID

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif			

#include <windows.h>
#include <setupapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <stdio.h>
#include <stdlib.h>

#define WHILE(a) \
while(__pragma(warning(disable:4127)) a __pragma(warning(disable:4127)))

//
// Define an Interface Guid so that app can find the device and talk to it.
//
DEFINE_GUID (GUID_DEVINTERFACE_CC2430CABLE,
    0xA4D15FE6, 0xF1A5, 0x4e31, 0x97, 0xEC, 0xC1, 0xE3, 0x4A, 0x61, 0x28, 0x46);
// {A4D15FE6-F1A5-4e31-97EC-C1E34A612846}

#define IOCTL_CC2430_PPORT_INIT\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x801, METHOD_NEITHER,\
         FILE_READ_DATA)

#define IOCTL_CC2430_PPORT_END\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x802, METHOD_NEITHER,\
         FILE_READ_DATA)

#define IOCTL_CC2430_CHECK\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x803, METHOD_BUFFERED,\
         FILE_READ_DATA)
