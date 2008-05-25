//
//    CC2430Cable parallel interface driver
//
//    ParPort.h
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

#define REGISTER_DATA		0x00
#define REGISTER_STATUS		0x01
#define REGISTER_CONTROL	0x02

NTSTATUS ParPortInit(
					 IN PCWSTR ParallelPortName,
					 IN PDEVICE_CONTEXT DeviceContext
					 );

NTSTATUS static ParPortCheckCable(
					 IN PDEVICE_CONTEXT DeviceContext
					 );

NTSTATUS ParPortAllocate(
						 PDEVICE_CONTEXT DeviceContext
						 );


NTSTATUS ParPortFree(
					 PDEVICE_CONTEXT DeviceContext
					 );

NTSTATUS ParPortClearMode(
						IN PDEVICE_CONTEXT DeviceContext
						);

NTSTATUS ParPortSetByteMode(
						IN PDEVICE_CONTEXT DeviceContext
						);

NTSTATUS ParPortWriteByte( IN PDEVICE_CONTEXT DeviceContext,
							  IN UCHAR Address,
							  IN UCHAR Byte);

NTSTATUS ParPortReadByte( IN PDEVICE_CONTEXT DeviceContext,
							  IN UCHAR Address,
							  PUCHAR Byte);
