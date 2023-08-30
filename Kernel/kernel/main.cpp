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
//#define DEBUG_PRINT( msg, ... ) ;

NTSTATUS DeviceControl( PDEVICE_OBJECT DeviceObject, PIRP Irp ) {
    NTSTATUS status{ STATUS_SUCCESS };

    const auto irpSp{ IoGetCurrentIrpStackLocation( Irp ) };
    const auto comms{ ( CommsParse_t* ) Irp->AssociatedIrp.SystemBuffer };

    switch ( irpSp->Parameters.DeviceIoControl.IoControlCode ) {
    case IOCTL_NUMBER:
        DEBUG_PRINT( "[ HAVOC ] Received comms\n" );
        Communication::CommunicationBuffer = reinterpret_cast< char* >( comms->m_pBuffer );
        Communication::GamePID = reinterpret_cast< HANDLE >( comms->m_pGameProcessId );
        Communication::ClientPID = reinterpret_cast< HANDLE >( comms->m_pClientProcessId );

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


VOID WorkerThread( PVOID StartContext ) {
    while ( !unload ) {
        if ( !loaded )
            continue;

        if ( !Communication::CommunicationBuffer ) {
            //DEBUG_PRINT( "[ HAVOC ] Comm buffer not initialised\n" );
            continue;
        }

        DataRequest_t req{ };
        SIZE_T read;
        if ( Memory::ReadProcessMemory( Communication::ClientPID, Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ), &read ) != STATUS_SUCCESS )
            continue;

        if ( req.m_iType && req.m_pBuffer && req.m_nSize ) {
            const auto buf{ ( char* ) ExAllocatePool2( POOL_FLAG_NON_PAGED, req.m_nSize, 'HVC' ) };
            if ( !buf )
                continue;

            switch ( req.m_iType ) {
            case REQUEST_READ:
                Memory::ReadProcessMemory( Communication::GamePID, req.m_pAddress, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( Communication::ClientPID, req.m_pBuffer, buf, req.m_nSize, &read );
                break;
            case REQUEST_WRITE:
                // TODO: is this correct?
                Memory::ReadProcessMemory( Communication::ClientPID, req.m_pBuffer, buf, req.m_nSize, &read );
                Memory::WriteProcessMemory( Communication::GamePID, req.m_pAddress, buf, req.m_nSize, &read );
                break;
            }

            ExFreePool( buf );

            req.m_iType = 0;
            Memory::WriteProcessMemory( Communication::ClientPID, Communication::CommunicationBuffer, &req, sizeof( DataRequest_t ), &read );

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

    status = PsCreateSystemThread( &hThread, THREAD_ALL_ACCESS, NULL, NULL, NULL, WorkerThread, NULL );
    if ( NT_SUCCESS( status ) )
        ZwClose( hThread );

    return STATUS_SUCCESS;
}