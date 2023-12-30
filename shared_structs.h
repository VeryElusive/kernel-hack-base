
struct DataRequest_t {
    volatile int m_iType{ };
    void* m_pAddress{ };
    void* m_pBuffer{ };
    volatile int m_nSize{ };
};

enum {
    REQUEST_READ = 1,
    REQUEST_WRITE,
    REQUEST_GET_PID,
    REQUEST_GET_PROCESS_BASE,
    REQUEST_GET_MODULE_BASE
};

struct CommsParse_t {
    HANDLE m_pClientProcessId;
    //wchar_t* m_pGameProcessId;
    //int m_iEntryDeltaFromBase;
    //int m_iDriverSize;
    DataRequest_t* m_pBuffer;
};