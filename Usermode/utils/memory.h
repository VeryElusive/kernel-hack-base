#pragma once
#include "../context.h"

// TODO: test performance of this cuz it could be poor due to timing issues (doubt)

namespace Memory {
    void Write( void* address, void* buffer, int size ) {
        Context::CommunicationBuffer.m_iType = REQUEST_WRITE;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { };
    }

    void Read( void* address, void* buffer, int size ) {
        Context::CommunicationBuffer.m_iType = REQUEST_READ;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        while ( Context::CommunicationBuffer.m_iType != 0 ) { };
    }

    void UnloadDriver( ) {
        Context::CommunicationBuffer.m_iType = 0xFADED;
        Context::CommunicationBuffer.m_nSize = 0xFADED;
    }
}