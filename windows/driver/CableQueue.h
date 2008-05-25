//
//    CC2430Cable parallel interface driver
//
//    CableQueue.h
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

#define MAX_WRITE_LENGTH 1024*4

#define TIMER_PERIOD 1000*10

//
// Control Code Definition
// Will be used by user mode applications to send and receive informations
//
#define IOCTL_CC2430_PPORT_INIT\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x801, METHOD_NEITHER,\
         FILE_READ_DATA)

#define IOCTL_CC2430_PPORT_END\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x802, METHOD_NEITHER,\
         FILE_READ_DATA)

#define IOCTL_CC2430_CHECK\
   CTL_CODE(FILE_DEVICE_PARALLEL_PORT, 0x803, METHOD_BUFFERED,\
         FILE_READ_DATA)

typedef struct _QUEUE_CONTEXT {
	//Buffer
	PVOID Buffer;
	ULONG Length;

	//Timer for this queue
	WDFTIMER Timer;

	// Virtual I/O
	WDFREQUEST CurrentRequest;
	NTSTATUS CurrentStatus;

}QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS CableQueueInitialize(
							  WDFDEVICE hDevice
							  );

VOID CableEvtIoQueueContextDestroy(
								   WDFOBJECT Object
								   );

VOID CableEvtRequestCancel(
						   IN WDFREQUEST Request
						   );

VOID CableEvtIoRead(
					IN WDFQUEUE		Queue,
					IN WDFREQUEST	Request,
					IN size_t		Length
					);

VOID CableEvtIoWrite(
					 IN WDFQUEUE	Queue,
					 IN WDFREQUEST	Request,
					 IN size_t		Length
					 );

VOID CableEvtIoDeviceControl(
							 IN WDFQUEUE Queue,
							 IN WDFREQUEST Request,
							 IN size_t OutputBufferSize,
							 IN size_t InputBufferSize,
							 IN ULONG IoControlCode
							 );

NTSTATUS CableTimerCreate(
						  IN WDFTIMER *	pTimer,
						  IN ULONG		Period,
						  IN WDFQUEUE	Queue
						  );

VOID CableEvtTimerFunc(
					   IN WDFTIMER	Timer
					   );
