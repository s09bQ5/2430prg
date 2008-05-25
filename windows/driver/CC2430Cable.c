//
//    CC2430Cable parallel interface driver
//
//    CC2430Cable.c
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
#include "driver.h"

NTSTATUS wait(ULONG uSec)
{
	LARGE_INTEGER waitFor;
	KTIMER timer;
	NTSTATUS status;

	waitFor.QuadPart = -10 * uSec;
	KeInitializeTimer(&timer);
	KeSetTimer(&timer,waitFor,NULL);
	status = KeWaitForSingleObject(&timer, Executive, KernelMode,FALSE,NULL);
	return status;
}

NTSTATUS CC2430CheckCable( 
						  PDEVICE_CONTEXT DeviceContext,
						  PCC2430_CABLE_STATUS Status
						  )
{
	NTSTATUS status = STATUS_SUCCESS;
	UCHAR byte = 0;

	if(Status == NULL){
		return STATUS_INVALID_PARAMETER;
	}

	Status->version = 0;
	Status->deviceConnected = FALSE;

	KdPrint(("--> CC2430CheckCable\n"));

	status = ParPortWriteByte(DeviceContext,REGISTER_DATA,~CC2430_D6);
	if(NT_SUCCESS(status)){

		//
		// We will wait 20 mSec before read it back
		//
		wait(20000);

		//
		// Read the status register and check the cable version and VCC_SENSE
		//
		status = ParPortReadByte(DeviceContext,REGISTER_STATUS,&byte);
		if(NT_SUCCESS(status)){
			if( (byte & CC2430_CABLE_V1_MASK) == CC2430_CABLE_V1){
				Status->version = 1;
			}

			if(byte & CC2430_VCC_SENSE){
				Status->deviceConnected = TRUE;
			}

			KdPrint(("Status=0x%x\n",Status->version));
		}
	}

	//
	// Everything goes up
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,0xff);

	KdPrint(("<-- CC2430CheckCable\n"));
	return status;
}

NTSTATUS CC2430StartDebugMode(
							  PDEVICE_CONTEXT DeviceContext
							 )
{
	UCHAR byte = 0;
	UCHAR i = 0;

	do{
		//
		// Reset and clock bit goes down
		//
		if(byte == 0 || byte == ~CC2430_RESET)
			byte =  ~CC2430_RESET & ~CC2430_CLK; // reset and clock goes down
		else
			byte = ~CC2430_RESET; // reset keeps down and clock goes up

		ParPortWriteByte(DeviceContext,REGISTER_DATA,byte);

		//
		// Wait 10mSec before change it again
		//
		wait(10000);
	} while(i++ < 3);

	//
	// Evething goes up and we are in debug mode
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,0xff);

	return STATUS_SUCCESS;
}

NTSTATUS CC2430Reset(
					 PDEVICE_CONTEXT DeviceContext
					) 
{
	//
	//Reset goes down
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,~CC2430_RESET);

	//
	// Wait 10mSec before change it again
	//
	wait(10000);

	//
	// Everything goes up
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,0xff);
}

static NTSTATUS CC2430WriteByte(
							   PDEVICE_CONTEXT DeviceContext,
							   UCHAR Byte
							   )
{
	NTSTATUS status;
	UCHAR i;
	UCHAR bit;
	UCHAR outputByte = 0xFF;

	//
	// PROG pin must goes down when writing
	//
	outputByte = ~CC2430_PROG;

	for(i=0; i<8; i++){
		
		bit = Byte & 0x01;

		//
		// DIN bit goes to 0
		//
		outputByte &= ~CC2430_DIN;
		outputByte |= bit;

		//
		//Write the bit out
		//
		ParPortWriteByte(DeviceContext,REGISTER_DATA,outputByte);

		//
		// Wait 1uSec before down the clock
		//
		wait(1);

		outputByte &= ~CC2430_CLK; // down the clock
		ParPortWriteByte(DeviceContext,REGISTER_DATA,outputByte);

		//
		// Wait 1uSec before up the clock
		//
		wait(1);

		outputByte |= CC2430_CLK; // up the clock
		ParPortWriteByte(DeviceContext,REGISTER_DATA,outputByte);

		//
		// Next Bit
		//
		Byte >>= 1;
	}

	//
	// Everything goes up
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,0xff);

	return STATUS_SUCCESS;
}

static NTSTATUS CC2430ReadByte(
							   PDEVICE_CONTEXT DeviceContext,
							   PUCHAR Byte
							  )
{
	NTSTATUS status;
	UCHAR i;
	UCHAR bit;
	UCHAR outputByte = 0xFF;

	if(Byte == NULL)
		return STATUS_INVALID_PARAMETER;

	*Byte = 0x00;

	for(i = 0; i < 8; i++){

		outputByte &= ~CC2430_CLK; // down the clock
		ParPortWriteByte(DeviceContext,REGISTER_DATA,outputByte);

		//
		// Wait 1uSec before read the data
		//
		wait(1);

		ParPortReadByte(DeviceContext,REGISTER_STATUS,&bit);
		*Byte |= (bit & CC2430_DONE);
		*Byte <<= 1;

		//
		// Wait 1uSec before up the clock
		//
		wait(1);

		outputByte |= CC2430_CLK; // up the clock
		ParPortWriteByte(DeviceContext,REGISTER_DATA,outputByte);

		//
		// Wait 1uSec before down the clock again
		//
		wait(1);
	}

	//
	// Everything goes up
	//
	ParPortWriteByte(DeviceContext,REGISTER_DATA,0xff);

	return STATUS_SUCCESS;
}

NTSTATUS CC2430SendCommand(
						   PDEVICE_CONTEXT DeviceContext,
						   UCHAR Command,
						   UCHAR Params[3],
						   PUCHAR Return
						  )
{
	NTSTATUS status;
	UCHAR i;
	UCHAR nParams;

	//
	// Write the command
	//
	status = CC2430WriteByte(DeviceContext,Command);

	if(NT_SUCCESS(status)){
		//
		// Number of output bytes supplied with this command
		//
		nParams = Command & DEBUG_COMMAND_OUTPUT_BYTES;
		for(i = 0; i < nParams; i++){
			CC2430WriteByte(DeviceContext,Params[i]);
		}

		//
		// Check if this command send to us something
		//
		if(Command & DEBUG_COMMAND_INPUT_BYTES)
			CC2430ReadByte(DeviceContext,Return);
		else
			*Return = 0x00;
	}

	return status;
}