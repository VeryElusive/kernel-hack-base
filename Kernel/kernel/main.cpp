#pragma warning ( disable : 4100 )

#include "utils/messages.h"
#include "utils/memory.h"
#include "communication/communication.h"
#include <ntifs.h>

#include "sdk/windows/ntapi.h"

struct CommsParse_t {
    PVOID m_pProcessId;
    char* m_pBuffer;
};

bool unload;
bool start{ };

#define IOCTL_NUMBER 0xFADED


NTSTATUS DeviceControl( PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    NTSTATUS status{ STATUS_SUCCESS };

    const auto irpSp{ IoGetCurrentIrpStackLocation( Irp ) };
    const auto comms{ ( CommsParse_t* ) Irp->AssociatedIrp.SystemBuffer };

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
    case IOCTL_NUMBER:
        DEBUG_PRINT( "[ HAVOC ] Received comms\n" );
        Communication::CommunicationBuffer = comms->m_pBuffer;

        status = PsLookupProcessByProcessId( comms->m_pProcessId, &Communication::Process );
        start = true;

        if ( !NT_SUCCESS( status ) ) {
            DEBUG_PRINT( "[ HAVOC ] Failed to find process\n" );
            break;
        }

        DEBUG_PRINT( "[ HAVOC ] Successfully parsed comms\n" );

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

HANDLE hThread;

VOID DriverUnload( PDRIVER_OBJECT DriverObject ) {
    UNICODE_STRING dosDeviceName;

    RtlInitUnicodeString( &dosDeviceName, L"\\DosDevices\\Havoc" );
    IoDeleteSymbolicLink( &dosDeviceName );
    IoDeleteDevice( DriverObject->DeviceObject );

    unload = true;
    ZwClose( hThread );

    DEBUG_PRINT( "[ HAVOC ] Unloaded driver\n" );
}


VOID ThreadFunction( PVOID StartContext ) {
    //size_t bytes{ };
    while ( !unload ) {
        if ( !Communication::CommunicationBuffer || !Communication::ControlProcess || !Communication::GameProcess ) {
            if ( start )
                DEBUG_PRINT( "[ HAVOC ] Comm buffer not initialised\n" );
            continue;
        }

        DataRequest_t req{ };
        if ( !Memory::Read( Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ) ) ) {
            DEBUG_PRINT( "[ HAVOC ] failed to read comm buffer\n" );
            continue;
        }

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            DEBUG_PRINT( "[ HAVOC ] found type!\n" );
            // Memory::Write( req.m_pAddress, req.m_pBuffer, req.m_nSize );

            char buf[ 4 ];
            Memory::Read( req.m_pBuffer, buf, req.m_nSize );

            /*switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::Read( req.m_pAddress, req.m_pBuffer, req.m_nSize );
                break;
            case REQUEST_WRITE:
                break;
            }*/

            Memory::Write( req.m_pAddress, buf, req.m_nSize );
            req.m_iType = 0;
            Memory::Write( Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ) );

            //MmCopyVirtualMemory( IoGetCurrentProcess( ), buf, Communication::Process, req.m_pAddress, req.m_nSize, KernelMode, &bytes );
            break;
        }
    }
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

    status = PsCreateSystemThread( &hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, ThreadFunction, NULL );
    if ( NT_SUCCESS( status ) )
        ZwClose( hThread );

    return STATUS_SUCCESS;
}