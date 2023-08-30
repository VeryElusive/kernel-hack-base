#pragma once
#include "../context.h"

namespace Memory {
    void Write( void* address, void* buffer, int size ) {
        Context::CommunicationBuffer.m_iType = REQUEST_WRITE;
        Context::CommunicationBuffer.m_pAddress = address;
        Context::CommunicationBuffer.m_pBuffer = buffer;
        Context::CommunicationBuffer.m_nSize = size;

        // TODO: test performance of this
        while ( Context::CommunicationBuffer.m_iType != 0 ) { };
    }
}