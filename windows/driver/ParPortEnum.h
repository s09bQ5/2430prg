//
//    CC2430Cable parallel interface driver
//
//    ParPortEnum.h
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

typedef struct _PARPORT_ENUMERATE_WDM
{
	//
	// Pointer to a list of symbolic links which
	// the drivers names which implements the
	// parallel port interface
	//
	PWSTR SymbolicLinkList;

	//
	// Pointer to the current entry on the SymbolicLinkList
	//
	PWSTR CurrentSymbolicLink;
} PARPORTENUM,*PPARPORTENUM;

NTSTATUS ParPortEnumerateOpen(PPARPORTENUM * enumerate);

NTSTATUS ParPortEnumerate(PPARPORTENUM EnumStruct, PCWSTR* DriverName);

NTSTATUS ParPortEnumerateClose(PPARPORTENUM EnumStruct);