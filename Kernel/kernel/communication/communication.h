#pragma once
#include <ntifs.h>

namespace Communication {
    inline char* CommunicationBuffer;
    inline PEPROCESS ControlProcess;
    inline PEPROCESS GameProcess;
    inline HANDLE PID;
}