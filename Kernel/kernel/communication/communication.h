#pragma once
#include <ntifs.h>
// TYPE: 1 BYTE
// ADDRESS: 4 BYTES
// BUFFER: 4 BYTES
// SIZE: 4 BYTES

struct DataRequest_t {
    char m_iType{ };
    void* m_pAddress{ };
    void* m_pBuffer{ };
    int m_nSize{ };
};

struct WriteDataRequest_t {
    int m_pProcessId;
    PVOID m_pAddress;
    PVOID m_pBuffer;
    SIZE_T m_nSize;
};

namespace Communication {
	char* Communication;

    int PID;
}

