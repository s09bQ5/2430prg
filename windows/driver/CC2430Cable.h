//
//    CC2430Cable parallel interface driver
//
//    CC2430Cable.h
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

//
// STATUS REGISTER BIT POSITION AND MASK
//
#define CC2430_CABLE_V1_MASK			0xA0
#define CC2430_CABLE_V1					0x80
#define CC2430_VCC_SENSE				0x08

//
// DATA REGISTER BIT POSITION
//
#define CC2430_D6						0x40
#define CC2430_RESET					0x08
#define CC2430_CLK						0x02
#define CC2430_PROG						0x10
#define CC2430_DIN						0x01

//
// STATUS REGISTER BIT POSITION
//
#define CC2430_DONE						0x10

//
// COMMAND MASKS
//
#define DEBUG_COMMAND_OUTPUT_BYTES		0x3
#define DEBUG_COMMAND_INPUT_BYTES		0x4
#define DEBUG_COMMAND_CODE				0xF8

typedef struct _CC2430_CABLE_STATUS{
	//
	// Cable Hardware Version
	//
	UCHAR version;
	//
	// If the CC2430 development board is connected and powered up
	//
	BOOLEAN deviceConnected;
} CC2430_CABLE_STATUS, *PCC2430_CABLE_STATUS;

NTSTATUS CC2430CheckCable( 
						  PDEVICE_CONTEXT DeviceContext,
						  PCC2430_CABLE_STATUS Status
						  );

NTSTATUS CC2430SendCommand(
						   PDEVICE_CONTEXT DeviceContext,
						   UCHAR Command,
						   UCHAR Params[3],
						   PUCHAR Return
						  );
