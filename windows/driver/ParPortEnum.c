//
//    CC2430Cable parallel interface driver
//
//    ParPortEnum.c
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

//
// Start enumerating the drivers that implements the parallel port interface
//
// \param enumerate
//			A pointer to an PPARPORTENUM that will hold the drivers names
//
NTSTATUS ParPortEnumerateOpen(PPARPORTENUM * enumerate)
{
    PPARPORTENUM enumStruct;
	NTSTATUS status;

	KdPrint( ("--> ParPortEnumerateOpen\n") );

	//
	// Get the memory we need to hold and manage the parallel port information
	//
	enumStruct = ExAllocatePoolWithTag(PagedPool, sizeof(PARPORTENUM),'enu');

	if(enumStruct){
		enumStruct->CurrentSymbolicLink = NULL;
		//
		// Get all drivers names which implements the parallel port interface
		//
		status = IoGetDeviceInterfaces(&GUID_PARALLEL_DEVICE, NULL, 0, &enumStruct->SymbolicLinkList);
	} else {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// if failed, we need to clean the memory
	if(!NT_SUCCESS(status)){
		if(enumStruct){
			ExFreePool(enumStruct);
			enumStruct = NULL;
		}
	}

	*enumerate = enumStruct;

	KdPrint( ("<-- ParPortEnumerateOpen\n") );

	return status;
}

//
// Get the next driver name
//
// \param EnumStruct
//			PARPORTENUM with the drivers list
//
// \param DriverName
//			A pointer pointing to the buffer which will receive the next driver name
//
NTSTATUS ParPortEnumerate(PPARPORTENUM EnumStruct, PCWSTR* DriverName)
{
	NTSTATUS status;

	KdPrint( ("--> ParPortEnumerate\n") );

	if(EnumStruct->CurrentSymbolicLink == NULL){
		EnumStruct->CurrentSymbolicLink = EnumStruct->SymbolicLinkList;
	} else {
		EnumStruct->CurrentSymbolicLink += wcslen(EnumStruct->CurrentSymbolicLink)+1;
	}

	*DriverName = EnumStruct->CurrentSymbolicLink;

	status = ( (**DriverName) ? STATUS_SUCCESS : STATUS_NO_MORE_ENTRIES);

	KdPrint( ("<-- ParPortEnumerate\n") );
	return status;
}

//
// Free the resources used to enumerating the parallel port drivers
//
// \param EnumStruct
//			PARPORTENUM with the drivers list
//
NTSTATUS ParPortEnumerateClose(PPARPORTENUM EnumStruct)
{
	KdPrint( ("--> ParPortEnumerateClose\n") );

	ExFreePool(EnumStruct->SymbolicLinkList);
	ExFreePool(EnumStruct);

	KdPrint( ("<-- ParPortEnumerateClose\n") );

	return STATUS_SUCCESS;
}