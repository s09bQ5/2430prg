//
//    CC2430Cable parallel interface driver
//
//    CableDevice.c
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
#pragma alloc_text(PAGE,CableDeviceCreate)
#pragma alloc_text(PAGE,CableEvtDeviceSelfManagedIoStart)
#pragma alloc_text(PAGE,CableEvtDeviceSelfManagedIoSuspend)
#endif

//
// Our Cable device interface GUID
//

NTSTATUS CableDeviceCreate(
						   IN PWDFDEVICE_INIT DeviceInit,
						   IN PCWSTR ParallelPortName,
						   IN ULONG DeviceNumber
						   )
{
	WDF_OBJECT_ATTRIBUTES deviceAttributes;
	PDEVICE_CONTEXT deviceContext;
	WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
	WDFDEVICE device;
	NTSTATUS status;
	UNICODE_STRING prefix;
	UNICODE_STRING number;
	UNICODE_STRING reference;
	WCHAR numberBuffer[50];
	WDFSTRING link;
	UNICODE_STRING strLink;

	PAGED_CODE();

	KdPrint(("--> CableDeviceCreate\n"));

	//
	// Unique device reference string prefix
	//
	RtlInitUnicodeString(&prefix,CC2430_DEVICE_NAME);

	number.Length = 0;
	number.MaximumLength = sizeof(numberBuffer);
	number.Buffer = numberBuffer;
	RtlIntegerToUnicodeString(DeviceNumber,10,&number);

	reference.Length = 0;
	reference.MaximumLength = prefix.Length + number.Length;
	reference.Buffer = ExAllocatePoolWithTag(NonPagedPool, reference.MaximumLength, 'ref');

	if(!reference.Buffer){
        status = STATUS_INSUFFICIENT_RESOURCES;
		KdPrint(("CableDeviceCreate failed with 0x%x\n",status));
		return status;
	}

	RtlCopyUnicodeString(&reference,&prefix);
	RtlAppendUnicodeStringToString(&reference,&number);

	//
	// PNP configurations
	//
	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

	//
	// Register pnp/power callbacks to start and stop the timer
	//
	pnpPowerCallbacks.EvtDeviceSelfManagedIoInit	= CableEvtDeviceSelfManagedIoStart;
	pnpPowerCallbacks.EvtDeviceSelfManagedIoSuspend = CableEvtDeviceSelfManagedIoSuspend;
	pnpPowerCallbacks.EvtDeviceSelfManagedIoRestart = CableEvtDeviceSelfManagedIoStart;

	//
	// Register the PnP and power callbacks
	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit,&pnpPowerCallbacks);

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

	//
	// WDFDEVICE synchronization scope
	//
	deviceAttributes.SynchronizationScope = WdfSynchronizationScopeDevice;
	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

	if(NT_SUCCESS(status)){
		//
		// Get the device context and initialize it
		//
		deviceContext = WdfObjectGet_DEVICE_CONTEXT(device);

		//
		// Get the parallel port information
		//
		status = ParPortInit(ParallelPortName,deviceContext);

		if(NT_SUCCESS(status)){
			//
			// Create a symbolic link to our device so that application can find and talk to us
			//
			status = WdfDeviceCreateDeviceInterface(device,
													(LPGUID)&GUID_DEVINTERFACE_CC2430CABLE,
													&reference);
			if(NT_SUCCESS(status)){

				status = WdfStringCreate(
										 WDF_NO_OBJECT_ATTRIBUTES,
										 NULL,
										 &link
										 );

				if(NT_SUCCESS(status)){
					status = WdfDeviceRetrieveDeviceInterfaceString(device,
															  (LPGUID)&GUID_DEVINTERFACE_CC2430CABLE,
															  &reference,
															  link);

					if(NT_SUCCESS(status)){
						WdfStringGetUnicodeString(link,&strLink);

						//
						// Initialize the I/O Package and any Queue
						//
						status = CableQueueInitialize(device);

						KdPrint(("Unique Reference String: %wZ\n",reference));
						KdPrint(("Symbolic link to this device: %wZ\n",strLink));

					}

				}
			}
		}

	}

	if(!NT_SUCCESS(status)){
		if(reference.Buffer)
			ExFreePool(reference.Buffer);

		KdPrint(("CableDeviceCreate failed with 0x%x\n",status));
	}

	KdPrint(("<-- CableDeviceCreate\n"));

	return status;
}

NTSTATUS CableEvtDeviceSelfManagedIoStart(
	IN WDFDEVICE hDevice
	)
{
	PQUEUE_CONTEXT queueContext = QueueGetContext(WdfDeviceGetDefaultQueue(hDevice));
	LARGE_INTEGER DueTime;

	PAGED_CODE();

	KdPrint(("--> CableEvtDeviceSelfManagedIoStart\n"));

	//
	// Restart the queue and the periodic timer. We stopped them before going into low power state.
	//
	WdfIoQueueStart(WdfDeviceGetDefaultQueue(hDevice));

	DueTime.QuadPart = WDF_REL_TIMEOUT_IN_MS(100);

	WdfTimerStart(queueContext->Timer, DueTime.QuadPart);

	KdPrint(("<-- CableEvtDeviceSelfManagedIoStart\n"));

	return STATUS_SUCCESS;
}

NTSTATUS CableEvtDeviceSelfManagedIoSuspend(
	IN WDFDEVICE hDevice
	)
{
	PQUEUE_CONTEXT queueContext = QueueGetContext(WdfDeviceGetDefaultQueue(hDevice));

	PAGED_CODE();

	KdPrint(("--> CableEvtDeviceSelfManagedIoSuspend\n"));

	WdfIoQueueStopSynchronously(WdfDeviceGetDefaultQueue(hDevice));
	WdfTimerStop(queueContext->Timer, TRUE);

	KdPrint(("<-- CableEvtDeviceSelfManagedIoSuspend\n"));

	return STATUS_SUCCESS;
}