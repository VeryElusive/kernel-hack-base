
struct DataRequest_t {
    volatile int m_iType{ };
    void* m_pAddress{ };
    void* m_pBuffer{ };
    int m_nSize{ };
};

enum {
    REQUEST_READ,
    REQUEST_WRITE,
};

struct CommsParse_t {
    int m_pClientProcessId;
    int m_pGameProcessId;
    int m_iEntryDeltaFromBase;
    int m_iDriverSize;
    DataRequest_t* m_pBuffer;
};