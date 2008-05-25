#include "CC2430CableTest.h"

PCHAR DevicePath;

PCHAR GetDevicePath(
    IN  LPGUID InterfaceGuid
    );

int __cdecl main(int argc, char* argv[])
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;
	DWORD bytesReturned;

	DevicePath = GetDevicePath((LPGUID)&GUID_DEVINTERFACE_CC2430CABLE);
	wprintf(L"DevicePath=%s\n",DevicePath);

	hDevice = CreateFile((LPCWSTR)DevicePath,
                         GENERIC_READ|GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open device. Error %d\n",GetLastError());
        goto exit;
    }

	if(DeviceIoControl(hDevice,IOCTL_CC2430_PPORT_INIT,NULL,0,NULL,0,&bytesReturned,NULL))
		printf("Print port initialized\n");

	if(DeviceIoControl(hDevice,IOCTL_CC2430_PPORT_END,NULL,0,NULL,0,&bytesReturned,NULL))
		printf("Print port closed\n");

	CloseHandle(hDevice);

exit:
	return 0;
}

PCHAR
GetDevicePath(
    IN  LPGUID InterfaceGuid
    )
{
    HDEVINFO HardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData = NULL;
    ULONG Length, RequiredLength = 0;
    BOOL bResult;

    HardwareDeviceInfo = SetupDiGetClassDevs(
                             InterfaceGuid,
                             NULL,
                             NULL,
                             (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (HardwareDeviceInfo == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs failed!\n");
        exit(1);
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    bResult = SetupDiEnumDeviceInterfaces(HardwareDeviceInfo,
                                              0,
                                              InterfaceGuid,
                                              0,
                                              &DeviceInterfaceData);

    if (bResult == FALSE) {

        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPTSTR)&lpMsgBuf,
                          0,
                          NULL
                          )) {

            printf("Error: %s", (LPSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }

        printf("SetupDiEnumDeviceInterfaces failed. %d\n",GetLastError());
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        exit(1);
    }

    SetupDiGetDeviceInterfaceDetail(
        HardwareDeviceInfo,
        &DeviceInterfaceData,
        NULL,
        0,
        &RequiredLength,
        NULL
        );

    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, RequiredLength);

    if (DeviceInterfaceDetailData == NULL) {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        printf("Failed to allocate memory.\n");
        exit(1);
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    Length = RequiredLength;

    bResult = SetupDiGetDeviceInterfaceDetail(
                  HardwareDeviceInfo,
                  &DeviceInterfaceData,
                  DeviceInterfaceDetailData,
                  Length,
                  &RequiredLength,
                  NULL);

    if (bResult == FALSE) {

        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&lpMsgBuf,
                      0,
                      NULL
                      )) {

            MessageBox(NULL, (LPCTSTR) lpMsgBuf, _T("Error"), MB_OK);
            LocalFree(lpMsgBuf);
        }
        printf("Error in SetupDiGetDeviceInterfaceDetail\n");

        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        LocalFree(DeviceInterfaceDetailData);
        exit(1);
    }

    return (PCHAR)DeviceInterfaceDetailData->DevicePath;
}
