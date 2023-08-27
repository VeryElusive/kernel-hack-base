#pragma warning ( disable : 4100 )

#include "utils/messages.h"
#include <ntifs.h>

#include "sdk/windows/ntapi.h"

struct WriteDataRequest_t {
    PVOID m_pProcessId;
    PVOID m_pAddress;
    PVOID m_pBuffer;
    SIZE_T m_nSize;
};

#define IOCTL_NUMBER 0xFADED

NTSTATUS DeviceControl( PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    PEPROCESS process;
    NTSTATUS status{ STATUS_SUCCESS };
    size_t bytes = 0;

    const auto irpSp{ IoGetCurrentIrpStackLocation( Irp ) };
    const auto request{ ( WriteDataRequest_t* ) Irp->AssociatedIrp.SystemBuffer };

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
    case IOCTL_NUMBER:
        DEBUG_PRINT( "[ HAVOC ] Received comms\n" );

        status = PsLookupProcessByProcessId( request->m_pProcessId, &process );
        if ( !NT_SUCCESS( status ) ) {
            DEBUG_PRINT( "[ HAVOC ] Failed to find process\n" );
            break;
        }

        status = MmCopyVirtualMemory( IoGetCurrentProcess( ), request->m_pBuffer, process, request->m_pAddress, request->m_nSize, KernelMode, &bytes );

        DEBUG_PRINT( "[ HAVOC ] Success\n" );
        break;
    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return status;
}

NTSTATUS CreateClose( PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

VOID DriverUnload( PDRIVER_OBJECT DriverObject ) {
    UNICODE_STRING dosDeviceName;

    RtlInitUnicodeString( &dosDeviceName, L"\\DosDevices\\Havoc" );
    IoDeleteSymbolicLink( &dosDeviceName );
    IoDeleteDevice( DriverObject->DeviceObject );

    DEBUG_PRINT( "[ HAVOC ] Unloaded driver\n" );
}

extern "C" NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath ) {
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING dosDeviceName;
    PDEVICE_OBJECT deviceObject;

    RtlInitUnicodeString( &deviceName, L"\\Device\\Havoc" );
    status = IoCreateDevice( DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject );
    if ( !NT_SUCCESS( status ) ) {
        DEBUG_PRINT( "[ HAVOC ] Failed to create device\n" );
        return status;
    }

    DEBUG_PRINT( "[ HAVOC ] Created device\n" );

    RtlInitUnicodeString( &dosDeviceName, L"\\DosDevices\\Havoc" );
    status = IoCreateSymbolicLink( &dosDeviceName, &deviceName );
    if ( !NT_SUCCESS( status ) ) {
        DEBUG_PRINT( "[ HAVOC ] Failed to create symbolic link\n" );
        IoDeleteDevice( deviceObject );
        return status;
    }

    DEBUG_PRINT( "[ HAVOC ] Loaded driver\n" );

    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = CreateClose;
    DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = CreateClose;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}