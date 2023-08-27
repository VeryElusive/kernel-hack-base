
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

struct CommsParse_t {
    int m_pProcessId;
    DataRequest_t* m_pBuffer;
};

#define IOCTL_NUMBER 0xFADED