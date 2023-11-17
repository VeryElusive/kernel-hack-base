#pragma once
#include "../context.h"
#include <iostream>
#include <intrin.h> 

// TODO: test performance of this cuz it could be poor due to timing issues (doubt)

namespace Memory {
    inline void Write( void* address, void* buffer, int size ) {
        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };

        Context::CommunicationBuffer.m_iType = REQUEST_WRITE;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };
    }

    inline void Read( void* address, void* buffer, int size ) {
        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };

        Context::CommunicationBuffer.m_iType = REQUEST_READ;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };
    }

    template <class T>
    __forceinline T Read( void* address ) {
        T ret{ };
        Read( address, &ret, sizeof( T ) );
        return ret;
    }

    inline void* GetModuleBase( const wchar_t* moduleName ) {
        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };

        void* buf{ };
        Context::CommunicationBuffer.m_iType = REQUEST_GET_MODULE_BASE;
        Context::CommunicationBuffer.m_pAddress = const_cast< wchar_t* >( moduleName );
        Context::CommunicationBuffer.m_pBuffer = &buf;
        Context::CommunicationBuffer.m_nSize = lstrlenW( moduleName );

        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };

        return buf;
    }

    inline void UnloadDriver( ) {
        Context::CommunicationBuffer.m_iType = 0xFADED;
        Context::CommunicationBuffer.m_nSize = 0xFADED;
    }    
    
    inline void WaitForGame( const char* game ) {
        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };

        Context::CommunicationBuffer.m_iType = REQUEST_GET_PID;
        Context::CommunicationBuffer.m_pAddress = const_cast< char* >( game );
        Context::CommunicationBuffer.m_nSize = strlen( game );

        while ( Context::CommunicationBuffer.m_iType != 0 ) { _ReadWriteBarrier( ); };
    }

    inline void WaitForDriver( ) {
        void* buf{ };
        Context::CommunicationBuffer.m_iType = 0xFADE;
        Context::CommunicationBuffer.m_nSize = 0xFADE;
        Context::CommunicationBuffer.m_pAddress = &buf;
        Context::CommunicationBuffer.m_pBuffer = &buf;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { Sleep( 500 ); _ReadWriteBarrier( ); };
    }
}