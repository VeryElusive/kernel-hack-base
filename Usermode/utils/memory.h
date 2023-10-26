#pragma once
#include "../context.h"

// TODO: test performance of this cuz it could be poor due to timing issues (doubt)

namespace Memory {
    inline void Write( void* address, void* buffer, int size ) {
        Context::CommunicationBuffer.m_iType = REQUEST_WRITE;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { };
    }

    inline void Read( void* address, void* buffer, int size ) {
        Context::CommunicationBuffer.m_iType = REQUEST_READ;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { };
    }

    template <class T>
    inline T Read( void* address ) {
        T ret{ };
        Read( address, &ret, sizeof( T ) );
        return ret;
    }

    inline void* GetModuleBase( const char* moduleName ) {
        // TODO:
        return 0;
    }

    inline void UnloadDriver( ) {
        Context::CommunicationBuffer.m_iType = 0xFADED;
        Context::CommunicationBuffer.m_nSize = 0xFADED;
    }
}