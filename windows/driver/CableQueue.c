//
//    CC2430Cable parallel interface driver
//
//    CableQueue.c
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CableQueueInitialize)
#pragma alloc_text(PAGE, CableTimerCreate)
#endif

NTSTATUS CableQueueInitialize(
							  WDFDEVICE hDevice
							  )
{
	WDFQUEUE	queue;
	NTSTATUS	status;
	PQUEUE_CONTEXT	queueContext;
	WDF_IO_QUEUE_CONFIG	queueConfig;
	WDF_OBJECT_ATTRIBUTES	queueAttributes;

	PAGED_CODE();

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,WdfIoQueueDispatchSequential);

	queueConfig.EvtIoRead = CableEvtIoRead;
	queueConfig.EvtIoWrite = CableEvtIoWrite;
	queueConfig.EvtIoDeviceControl = CableEvtIoDeviceControl;

	//
	// QUEUE_CONTEXT size and queue destroy callback
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&queueAttributes, QUEUE_CONTEXT);
	queueAttributes.EvtDestroyCallback = CableEvtIoQueueContextDestroy;

	status = WdfIoQueueCreate(
		hDevice,
		&queueConfig,
		&queueAttributes,
		&queue
		);

	if(!NT_SUCCESS(status)){
		KdPrint(("WdfIoQueueCreate failed 0x%x\n",status));
		return status;
	}

	//
	// Get our driver context memory from the returned Queue handle
	//
	queueContext = QueueGetContext(queue);
	queueContext->Buffer = NULL;
	queueContext->Timer = NULL;
	queueContext->CurrentRequest = NULL;
	queueContext->CurrentStatus = STATUS_INVALID_DEVICE_REQUEST;

	//
	// Create the timer queue
	//
	status = CableTimerCreate(&queueContext->Timer,TIMER_PERIOD, queue);
	if(!NT_SUCCESS(status)){
		KdPrint(("CableTimerCreate failed 0x%x\n",status));
		return status;
	}

	return status;
}

NTSTATUS CableTimerCreate(
						  WDFTIMER* pTimer,
						  ULONG Period,
						  WDFQUEUE Queue)
{
	NTSTATUS status;
	WDF_TIMER_CONFIG	timerConfig;
	WDF_OBJECT_ATTRIBUTES	timerAttributes;

	PAGED_CODE();

	WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, CableEvtTimerFunc, Period);

	WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
	timerAttributes.ParentObject = Queue;

	status = WdfTimerCreate(&timerConfig,&timerAttributes,pTimer);

	return status;
}

VOID CableEvtIoQueueContextDestroy(
								   WDFOBJECT Object
								   )
{
	PQUEUE_CONTEXT queueContext = QueueGetContext(Object);

	if(queueContext->Buffer != NULL){
		ExFreePool(queueContext->Buffer);
	}

	return;
}

VOID CableEvtRequestCancel(
						   WDFREQUEST Request
						   )
{
	PQUEUE_CONTEXT queueContext = QueueGetContext(WdfRequestGetIoQueue(Request));

	KdPrint(("CableEvtRequestCancel called on Request 0x%p\n",  Request));

	WdfRequestCompleteWithInformation(Request,STATUS_CANCELLED, 0L);

	ASSERT(queueContext->CurrentRequest == Request);
	queueContext->CurrentRequest = NULL;

	return;
}

VOID CableEvtIoRead(
					IN WDFQUEUE		Queue,
					IN WDFREQUEST	Request,
					IN size_t		pLength
					)
{
	NTSTATUS status;
	PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);
	WDFMEMORY memory;
	ULONG Length = pLength;

	KdPrint(("CableEvtIoRead Called! Queue 0x%p, Request 0x%p Length %d\n",
                            Queue,Request,Length));

	if( (queueContext->Buffer == NULL) ) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS,(ULONG_PTR)0L);
		return;
	}

	if(queueContext->Length < Length){
		Length = queueContext->Length;
	}

	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if(!NT_SUCCESS(status)){
        KdPrint(("CableEvtIoRead could not get request memory buffer 0x%x\n",status));
        WdfVerifierDbgBreakPoint();
        WdfRequestCompleteWithInformation(Request, status, 0L);
        return;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0,
		queueContext->Buffer,
		Length);

	if(!NT_SUCCESS(status)){
		KdPrint(("CableEvtIoRead: WdfMemoryCopyFromBuffer failed 0x%x\n",status));
        WdfRequestComplete(Request, status);
        return;
	}

	//
	//Set transfer information
	//
	WdfRequestSetInformation(Request,(ULONG_PTR)Length);

	//
	// Mark the request is cancelable
	//
	WdfRequestMarkCancelable(Request, CableEvtRequestCancel);

	//
	// Defer the completion to another thread from the timer dpc
	//
	queueContext->CurrentRequest	= Request;
	queueContext->CurrentStatus		= status;

	return;

}

VOID CableEvtIoWrite(
					 IN WDFQUEUE	Queue,
					 IN WDFREQUEST	Request,
					 IN size_t		Length
					 )
{
	NTSTATUS status;
	WDFMEMORY memory;
	PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);

	KdPrint(("CableEvtIoWrite Called! Queue 0x%p, Request 0x%p Length %d\n",
                            Queue,Request,Length));


	if(Length > MAX_WRITE_LENGTH) {
		KdPrint(("CableEvtIoWrite Buffer Length to big %d, Max is %d\n",Length,MAX_WRITE_LENGTH));
		WdfRequestCompleteWithInformation(Request, STATUS_BUFFER_OVERFLOW, 0L);
		return;
	}

	status = WdfRequestRetrieveInputMemory(Request,&memory);
	if(!NT_SUCCESS(status)){
        KdPrint(("CableEvtIoWrite could not get request memory buffer 0x%x\n",status));
        WdfVerifierDbgBreakPoint();
        WdfRequestComplete(Request, status);
        return;
	}

	if(queueContext->Buffer != NULL){
		ExFreePool(queueContext->Buffer);
		queueContext->Buffer = NULL;
		queueContext->Length = 0L;
	}

	queueContext->Buffer = ExAllocatePoolWithTag(NonPagedPool, Length, 'cab1');
	if(queueContext->Buffer == NULL){
        KdPrint(("CableEvtIoWrite: Could not allocate %d byte buffer\n",Length));
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
	}

	status = WdfMemoryCopyToBuffer(memory,0,queueContext->Buffer,Length);
	if(!NT_SUCCESS(status)){
        KdPrint(("CableEvtIoWrite WdfMemoryCopyToBuffer failed 0x%x\n", status));
        WdfVerifierDbgBreakPoint();
        WdfRequestComplete(Request, status);
        return;
	}

	queueContext->Length = (ULONG)Length;

	WdfRequestSetInformation(Request, (ULONG_PTR)Length);
	WdfRequestMarkCancelable(Request, CableEvtRequestCancel);

	queueContext->CurrentRequest = Request;
	queueContext->CurrentStatus = status;

	return;

}

VOID CableEvtIoDeviceControl(
							 IN WDFQUEUE Queue,
							 IN WDFREQUEST Request,
							 IN size_t OutputBufferSize,
							 IN size_t InputBufferSize,
							 IN ULONG IoControlCode
							 )
{
	//
	// Queue Context
	//
	PQUEUE_CONTEXT queueContext = QueueGetContext(Queue);
	//
	// Device Context
	//
	PDEVICE_CONTEXT deviceContext = WdfObjectGet_DEVICE_CONTEXT(WdfIoQueueGetDevice(Queue));
	//
	// Input and Output Memory
	//
	WDFMEMORY outputMemory;
	WDFMEMORY inputMemory;

	//
	// status
	//
	NTSTATUS status;

	ULONG length = 0;
	KIRQL currentIRQL, oldIRQL;

	KdPrint(("--> CableEvtIoDeviceControl\n"));

	switch(IoControlCode){
		//
		// Allocate the parallel port before start using it and
		// adjust the communication mode
		//
		case IOCTL_CC2430_PPORT_INIT:
			currentIRQL = KeGetCurrentIrql();
			KeLowerIrql(PASSIVE_LEVEL);
			status = ParPortAllocate(deviceContext);
			if(NT_SUCCESS(status)){
				status = ParPortClearMode(deviceContext);
				status = ParPortSetByteMode(deviceContext);
			}
			KeRaiseIrql(currentIRQL,&oldIRQL);
			break;

		//
		// Free the parallel port
		//
		case IOCTL_CC2430_PPORT_END:
			currentIRQL = KeGetCurrentIrql();
			KeLowerIrql(PASSIVE_LEVEL);
			status = ParPortClearMode(deviceContext);
			status = ParPortFree(deviceContext);
			KeRaiseIrql(currentIRQL,&oldIRQL);
			break;

		//
		// This function check if the write cable is connected to parallel port
		//
		case IOCTL_CC2430_CHECK:
			currentIRQL = KeGetCurrentIrql();
			KeLowerIrql(PASSIVE_LEVEL);
			//
			// Check if parallel port has been allocated to us and
			// byte mode has been setted before using it
			//
			if(deviceContext->ParallelPortAllocated && deviceContext->ParallelPortModeSetted){
				if(OutputBufferSize != sizeof(CC2430_CABLE_STATUS)){
					status = STATUS_INVALID_PARAMETER;
				} else {
					status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
					if(NT_SUCCESS(status)){
						CC2430_CABLE_STATUS cableStatus;
						status = CC2430CheckCable(deviceContext,&cableStatus);
						if(NT_SUCCESS(status)){
							WdfMemoryCopyFromBuffer(outputMemory,0,&cableStatus,sizeof(CC2430_CABLE_STATUS));
							length = sizeof(CC2430_CABLE_STATUS);
						}
					}
				}
			} else {
				KdPrint( ("CableEvtIoDeviceControl failed: Parallel port has not been allocated.\n") );
				status = STATUS_INVALID_PARAMETER;
			}
			KeRaiseIrql(currentIRQL,&oldIRQL);
			break;

		default:
			status = STATUS_INVALID_PARAMETER;
			break;
	}

	if(!NT_SUCCESS(status)){
        KdPrint(("CableEvtIoDeviceControl failed 0x%x\n", status));
        WdfVerifierDbgBreakPoint();
	}

	if(length > 0)
		WdfRequestCompleteWithInformation(Request, status, length);
	else
		WdfRequestComplete(Request, status);

	KdPrint(("<-- CableEvtIoDeviceControl\n"));
}

VOID CableEvtTimerFunc(
					   IN WDFTIMER	Timer
					   )
{
	NTSTATUS status;
	WDFREQUEST request;
	WDFQUEUE queue;
	PQUEUE_CONTEXT queueContext;

	queue = WdfTimerGetParentObject(Timer);
	queueContext = QueueGetContext(queue);

	request = queueContext->CurrentRequest;
	if(request != NULL){
		//
		//Attempt to remove cancel status from the request
		//
		status = WdfRequestUnmarkCancelable(request);
		if(status != STATUS_CANCELLED){

			queueContext->CurrentRequest = NULL;
			status = queueContext->CurrentStatus;

            KdPrint(("CustomTimerDPC Completing request 0x%p, Status 0x%x \n", request,status));

			WdfRequestComplete(request,status);
		} else {
            KdPrint(("CustomTimerDPC Request 0x%p is STATUS_CANCELLED, not completing\n", request));
		}
	}
}