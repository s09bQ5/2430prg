//
//    CC2430Cable parallel interface driver
//
//    ParPort.c
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
// Internal function only that will be used to send an IOCTL to parallel port driver
//
// \param DeviceContext
//			Pointer to out device context structure
//
// \param Ioctl
//			The IOCTL to perform
//
// \param InBuffer
//			Pointer to a buffer which holds the input
//
// \param InBufferLength
//			Length of the buffer pointed to by InBuffer
//
// \param OutBuffer
//			Pointer to a buffer which will get the output
//
// \param OutBufferLength
//			Length of the buffer pointed to by OutBuffer
//			Must be 0 if OutBuffer == NULL
static NTSTATUS
ParPortIoctlInOut(IN PDEVICE_CONTEXT DeviceContext, IN ULONG Ioctl,
                  IN PVOID InBuffer, IN ULONG InBufferLength,
				  OUT PVOID OutBuffer, IN ULONG OutBufferLength)
{
	NTSTATUS status;
	IO_STATUS_BLOCK ioStatusBlock;
	KEVENT event;
	PIRP irp;
	PIO_STACK_LOCATION stackLocation;

	if(KeGetCurrentIrql() <= APC_LEVEL){
		//
		// Event that we will use to be notified when  the IRP has finished
		//
		KeInitializeEvent(&event, NotificationEvent, FALSE);

		//
		// Build an IRP for this IOCTL
		//
		irp = IoBuildDeviceIoControlRequest(Ioctl,
												DeviceContext->ParallelPortDeviceObject,
												InBuffer,
												InBufferLength,
												OutBuffer,
												OutBufferLength,
												TRUE,
												&event,
												&ioStatusBlock);

		if( irp ){
			//
			// Get the current stack location
			//
			stackLocation = IoGetNextIrpStackLocation(irp);

			//
			// Reference the file object we are about to call.
			// This ensures the driver is not removed while we call it.
			// even if underlying hardware is removed
			//
			ObReferenceObject( DeviceContext->ParallelPortFileObject);

			//
			// Tell the IRP stack location to which file object we are
			// referring
			//
			stackLocation->FileObject = DeviceContext->ParallelPortFileObject;

			//
			// Call the parallel port lower level driver
			//
			status = IoCallDriver(DeviceContext->ParallelPortDeviceObject, irp);

			//
			// it's ok, we can dereference the file object now
			//
			ObDereferenceObject(DeviceContext->ParallelPortFileObject);

			if(!NT_SUCCESS(status)){
				KdPrint( ("IoCallDriver failed: 0x%x\n",status) );
			} else {
				//
				// We need to wait the IRP to be completed
				//
				status = KeWaitForSingleObject(&event,
												Executive,
												KernelMode,
												FALSE,
												NULL);

				if(!NT_SUCCESS(status)){
					KdPrint( ("KeWaitForSingleObject failed: 0x%x\n",status) );
				}
			}
		}
	} else {
		KdPrint( ("Invalid IRQL level: 0x%x - 0x%x\n",KeGetCurrentIrql(),APC_LEVEL) );
		status = STATUS_BAD_STACK;
	}
	return status;
}

NTSTATUS ParPortAllocate(
						 PDEVICE_CONTEXT DeviceContext
						 )
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("-->ParPortAllocate\n"));

	if(!DeviceContext->ParallelPortAllocated){
		status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE,NULL,0,NULL,0);

		if(NT_SUCCESS(status)){
			DeviceContext->ParallelPortAllocated = TRUE;
		}
	}

	KdPrint(("<--ParPortAllocate\n"));

	return status;
}

NTSTATUS ParPortFree(
					 PDEVICE_CONTEXT DeviceContext
					 )
{
	NTSTATUS status = STATUS_SUCCESS;

	KdPrint(("-->ParPortFree\n"));

	if(DeviceContext->ParallelPortAllocated){
		status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_PARALLEL_PORT_FREE,NULL,0,NULL,0);

		if(NT_SUCCESS(status)){
			DeviceContext->ParallelPortAllocated = FALSE;
		}
	}

	KdPrint(("<--ParPortFree\n"));

	return status;
}

//
// Parallel Port initialization function.
//
// Get the parallel port information by sending an IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO
// to the parallel port driver
//
// \param ParallelPortName
//			The parallel port name to retrieve informations
//
// \param DeviceContext
//			Pointer to out device context structure
//
NTSTATUS ParPortInit(
					 IN PCWSTR ParallelPortName,
					 IN PDEVICE_CONTEXT DeviceContext
					 )
{
	NTSTATUS status;
	UNICODE_STRING parallelPortName;

	KdPrint( ("-->ParPortInit\n") );

	//
	// Get the pointers to the parallel port pdo driver
	//
	parallelPortName.Buffer = (PWSTR)ParallelPortName;
	parallelPortName.Length = (USHORT)wcslen(ParallelPortName) * sizeof(WCHAR);
	parallelPortName.MaximumLength = parallelPortName.Length;

	status = IoGetDeviceObjectPointer(&parallelPortName,
										FILE_READ_ATTRIBUTES,
										&DeviceContext->ParallelPortFileObject,
										&DeviceContext->ParallelPortDeviceObject);

	if(!NT_SUCCESS(status)){
		KdPrint( ("IoGetDeviceObjectPointer failed: 0x%x\n",status) );
		return status;
	}

	//
	// Allocate memory to parallel information structure
	//
	DeviceContext->ParallelPortInformation = (PPARALLEL_PORT_INFORMATION)ExAllocatePoolWithTag(NonPagedPool,
																			sizeof(PARALLEL_PORT_INFORMATION),
																			'PPI');

	if( DeviceContext->ParallelPortInformation ){

		//
		// Send an IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO to the parallel port driver
		// and get the parallel port informations
		//
		status = ParPortIoctlInOut(DeviceContext, IOCTL_INTERNAL_GET_PARALLEL_PORT_INFO,
									NULL,0,
									DeviceContext->ParallelPortInformation,
									sizeof(PARALLEL_PORT_INFORMATION));

	} else {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if(NT_SUCCESS(status)){
		//
		// Save the parallel port registers I/O base address
		//
		DeviceContext->ParallelPortRegAddress = DeviceContext->ParallelPortInformation->Controller;
		DeviceContext->ParallelPortAllocated = FALSE;
		DeviceContext->ParallelPortModeSetted = FALSE;
		//
		// Dump the parallel port information
		//
        KdPrint(("Got parallel port information:\n"));
        KdPrint(("- OriginalController = 0x%p\n", DeviceContext->ParallelPortInformation->OriginalController));
        KdPrint(("- Controller         = 0x%p\n", DeviceContext->ParallelPortInformation->Controller));
        KdPrint(("- Span of controller = 0x%08x\n", DeviceContext->ParallelPortInformation->SpanOfController));
        KdPrint(("- TryAllocatePort    = 0x%p\n", DeviceContext->ParallelPortInformation->TryAllocatePort));
        KdPrint(("- FreePort           = 0x%p\n", DeviceContext->ParallelPortInformation->FreePort));
        KdPrint(("- QueryNumWaiters    = 0x%p\n", DeviceContext->ParallelPortInformation->QueryNumWaiters));
        KdPrint(("- Context            = 0x%p\n", DeviceContext->ParallelPortInformation->Context));
	} else {
		//
		// if failed getting the parallel port information we must free all allocated memory
		//
		if(DeviceContext->ParallelPortInformation){
			ExFreePool(DeviceContext->ParallelPortInformation);
			DeviceContext->ParallelPortInformation = NULL;
		}
	}

	KdPrint( ("<--ParPortInit\n") );

	return status;
}

NTSTATUS ParPortClearMode(
						IN PDEVICE_CONTEXT DeviceContext
						)
{
	NTSTATUS status;
	PARALLEL_PNP_INFORMATION parallelPnpInformation;
	PARALLEL_CHIP_MODE chipMode;

	KdPrint( ("-->ParPortClearMode\n") );

	//
	// Get the parallel port PNP information
	//
	status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO, NULL,0,&parallelPnpInformation,sizeof(PARALLEL_PNP_INFORMATION));

	if(NT_SUCCESS(status)){

		KdPrint( ("ParPortSetMode running at mode 0x%x\n",parallelPnpInformation.CurrentMode) );

		chipMode.ModeFlags = (UCHAR)parallelPnpInformation.CurrentMode;
		status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_PARALLEL_CLEAR_CHIP_MODE, &chipMode,sizeof(PARALLEL_CHIP_MODE),NULL,0);
		if(NT_SUCCESS(status))
			DeviceContext->ParallelPortModeSetted = FALSE;
	}

	KdPrint( ("<--ParPortClearMode\n") );

	return STATUS_SUCCESS;
}

NTSTATUS ParPortSetByteMode(
						IN PDEVICE_CONTEXT DeviceContext
						)
{
	NTSTATUS status;
	PARALLEL_PNP_INFORMATION parallelPnpInformation;
	PARALLEL_CHIP_MODE chipMode;

	KdPrint( ("-->ParPortSetMode\n") );

	//
	// Get the parallel port PNP information
	//
	status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_GET_PARALLEL_PNP_INFO, NULL,0,&parallelPnpInformation,sizeof(PARALLEL_PNP_INFORMATION));

	if(NT_SUCCESS(status)){

		KdPrint( ("ParPortSetMode running at mode 0x%x\n",parallelPnpInformation.CurrentMode) );

		//
		// Check if parallel port support the byte mode, otherelse,
		// it does not make sense to change the mode
		//
		if(parallelPnpInformation.HardwareCapabilities & PPT_BYTE_PRESENT){
			//
			// set the printer port mode as byte mode
			// (like standard mode but with bi-directional funcionality)
			//
			chipMode.ModeFlags = ECR_BYTE_MODE;
			status = ParPortIoctlInOut(DeviceContext,IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE, &chipMode,sizeof(PARALLEL_CHIP_MODE),NULL,0);

			if(!NT_SUCCESS(status)){
				KdPrint( ("IOCTL_INTERNAL_PARALLEL_SET_CHIP_MODE failed with 0x%x\n",status) );
			} else {
				KdPrint( ("ParPortSetByteMode: Parallel interface adjusted to byte mode\n") );
				DeviceContext->ParallelPortModeSetted = TRUE;
			}
		} else {
			KdPrint( ("Parallel port doesn´t support byte mode. How old is your computer?\n") );
			status = STATUS_NOT_SUPPORTED;
		}
	}

	KdPrint( ("<--ParPortSetMode\n") );

	return status;
}

NTSTATUS ParPortWriteByte( IN PDEVICE_CONTEXT DeviceContext,
							  IN UCHAR Address,
							  IN UCHAR Byte)
{
	//
	// The status register is a read only register
	//
	if(Address == REGISTER_STATUS)
		return STATUS_NOT_SUPPORTED;

	//
	// Parallel Port register address
	//
	WRITE_PORT_UCHAR(DeviceContext->ParallelPortRegAddress + Address,Byte);

	return STATUS_SUCCESS;
}

NTSTATUS ParPortReadByte( IN PDEVICE_CONTEXT DeviceContext,
							  IN UCHAR Address,
							  PUCHAR Byte)
{
	if(Byte == NULL){
		return STATUS_INVALID_PARAMETER;
	}

	*Byte = READ_PORT_UCHAR(DeviceContext->ParallelPortRegAddress + Address);

	return STATUS_SUCCESS;
}

