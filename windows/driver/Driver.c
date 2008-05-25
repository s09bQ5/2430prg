//
//    CC2430Cable parallel interface driver
//
//    Driver.c
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
#include "Driver.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#pragma alloc_text(PAGE,ParallelCableAdd)
#endif

NTSTATUS DriverEntry(
					 IN PDRIVER_OBJECT DriverObject,
					 IN PUNICODE_STRING RegistryPath
					 )
{
	WDF_DRIVER_CONFIG config;
	NTSTATUS status;

	WDF_DRIVER_CONFIG_INIT(&config, ParallelCableAdd);

	status = WdfDriverCreate(DriverObject,
							RegistryPath,
							WDF_NO_OBJECT_ATTRIBUTES,
							&config,
							WDF_NO_HANDLE);

	if(!NT_SUCCESS(status)){
		KdPrint( ("Error: WdfDriverCreate failed 0x%x\n",status) );
		return status;
	}

	return status;
}

NTSTATUS ParallelCableAdd(
						  IN WDFDRIVER Driver,
						  IN PWDFDEVICE_INIT DeviceInit
						  )
{
	NTSTATUS status;
	PPARPORTENUM enumerate;
	PCWSTR DriverName;
	ULONG i = 0;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	KdPrint( ("--> ParallelCableAdd\n") );

	//
	//Find all parallel port interfaces
	//
	status = ParPortEnumerateOpen(&enumerate);

	if(NT_SUCCESS(status)){
		do {
			status = ParPortEnumerate(enumerate,&DriverName);
			if(NT_SUCCESS(status) && *DriverName){
				//
				// Create the driver device for that parallel port interface
				//
				KdPrint( ("Drivername = %ws\n",DriverName) );
				status = CableDeviceCreate(DeviceInit, DriverName,i++);
			}
		} while( NT_SUCCESS(status) && *DriverName);

		//
		// Free all objects
		//
		ParPortEnumerateClose(enumerate);
	} else {
		KdPrint( ("ParPortEnumerateOpen failed with 0x%x\n",status) );
	}

	KdPrint( ("<-- ParallelCableAdd\n") );

	return STATUS_SUCCESS;
}