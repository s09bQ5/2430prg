//
//    CC2430Cable parallel interface driver
//
//    CableDevice.h
//    Copyright (C) 2007  Otavio Ribeiro
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#define WHILE(a) \
while(__pragma(warning(disable:4127)) a __pragma(warning(disable:4127)))

DEFINE_GUID (GUID_DEVINTERFACE_CC2430CABLE,
    0xA4D15FE6, 0xF1A5, 0x4e31, 0x97, 0xEC, 0xC1, 0xE3, 0x4A, 0x61, 0x28, 0x46);
// {A4D15FE6-F1A5-4e31-97EC-C1E34A612846}

#define CC2430_DEVICE_NAME L"CC2430Cable"

typedef struct _DEVICE_CONTEXT
{
	//
	// Parallel port lower level driver objects
	//
	PFILE_OBJECT ParallelPortFileObject;
	PDEVICE_OBJECT ParallelPortDeviceObject;

	//
	// Parallel port information
	//
	PPARALLEL_PORT_INFORMATION ParallelPortInformation;

	//
	// Parallel port system-mapped base I/O register location
	//
	PUCHAR ParallelPortRegAddress;

	//
	// Parallel port allocation flag
	//
	BOOLEAN ParallelPortAllocated;

	//
	// Parallel port mode set flag
	//
	BOOLEAN ParallelPortModeSetted;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE(DEVICE_CONTEXT)

//
// Function to initialize the cable device and its callbacks
//
NTSTATUS CableDeviceCreate(
						   IN PWDFDEVICE_INIT DeviceInit,
						   IN PCWSTR ParallelPortName,
						   IN ULONG DeviceNumber
						   );

//
//Device events
//
VOID CableEvtDeviceContextCleanup(
								  IN WDFDEVICE hDevice
								  );

NTSTATUS CableEvtDeviceSelfManagedIoStart(
										  IN WDFDEVICE hDevice
								          );

NTSTATUS CableEvtDeviceSelfManagedIoSuspend(
										    IN WDFDEVICE hDevice
								            );

