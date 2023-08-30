#pragma warning ( disable : 4100 )

#include "utils/memory.h"
#include "communication/communication.h"
#include <ntifs.h>

#include "sdk/windows/ntapi.h"
#include "../../shared_structs.h"

bool unload;
bool loaded{ };

#define IOCTL_NUMBER 0xFADED
#define DEBUG_PRINT( msg, ... ) DbgPrintEx( 0, 0, msg, __VA_ARGS__ );

NTSTATUS DeviceControl( PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    NTSTATUS status{ STATUS_SUCCESS };

    const auto irpSp{ IoGetCurrentIrpStackLocation( Irp ) };
    const auto comms{ ( CommsParse_t* ) Irp->AssociatedIrp.SystemBuffer };

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
    case IOCTL_NUMBER:
        DEBUG_PRINT( "[ HAVOC ] Received comms\n" );
        Communication::CommunicationBuffer = reinterpret_cast< char* >( comms->m_pBuffer );
        Communication::PID = reinterpret_cast< HANDLE >( comms->m_pProcessId );

        loaded = true;

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
        if ( !loaded )
            continue;

        if ( !Communication::CommunicationBuffer ) {
            //DEBUG_PRINT( "[ HAVOC ] Comm buffer not initialised\n" );
            continue;
        }

        DataRequest_t req{ };
        SIZE_T read;
        if ( Memory::ReadProcessMemory( Communication::PID, Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            char buf[ 4 ];

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( Communication::PID, req.m_pAddress, buf, 4, &read );
                Memory::WriteProcessMemory( Communication::PID, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                Memory::ReadProcessMemory( Communication::PID, req.m_pBuffer, buf, 4, &read );
                Memory::WriteProcessMemory( Communication::PID, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            }


            req.m_iType = 0;
            Memory::WriteProcessMemory( Communication::PID, Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ), &read );

            DEBUG_PRINT( "[ HAVOC ] wrote to buffer\n" );
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