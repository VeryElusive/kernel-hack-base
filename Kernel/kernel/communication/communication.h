#pragma once
#include <ntifs.h>

struct DataRequest_t {
    char m_iType{ };
    void* m_pAddress{ };
    void* m_pBuffer{ };
    int m_nSize{ };
};

enum {
    REQUEST_READ,
    REQUEST_WRITE,
};

namespace Communication {
    inline char* CommunicationBuffer;
    inline PEPROCESS Process;
}