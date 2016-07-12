#ifndef _FILE_MONITOR_
#define _FILE_MONITOR_
#include <Windows.h>
#include "Threads.h"
#include "Notification.h"
using namespace std;

class TCompletionPort
{
    typedef HANDLE TCompPortHandle;
    TCompPortHandle m_hCompletionPort;
public:
    enum COMPLETION_KEY_STATUS
    {
        COMPLETION_KEY_NONE = 0 ,
        COMPLETION_KEY_SHUTDOWN = 1 ,
    };
    struct TIOContext
    {
        DWORD m_TransferredBytes = 0;
        ULONG_PTR m_CompletionKey = 0;
        LPOVERLAPPED m_pOverlapped = 0;
        DWORD m_WaitDuration = INFINITE;
    };
    TCompletionPort ( unsigned long NumOfThreads = 0 );
    ~TCompletionPort ( );
    void  RegisterHandle ( HANDLE hDevice , ULONG_PTR completionKey );
    bool  GetIOPacket ( TCompletionPort::TIOContext& context );
    bool  PostIOPacket ( DWORD numOfBytes , ULONG_PTR dwCompletionKey , LPOVERLAPPED lpOverlapped );
    bool  GetIOPacket ( LPDWORD numOfBytes , PULONG_PTR dwCompletionKey , OVERLAPPED* overlapped , DWORD waitTime );
};
class TIOCompletionPortSystem : public TCompletionPort
{ //
    std::unique_ptr<TIOAsynchronousThreadPool> m_pThreadPool;
public:
    static TIOCompletionPortSystem& instance ( );
private:
    TIOCompletionPortSystem ( );
    virtual ~TIOCompletionPortSystem ( );
};
class TIOContext;
class TFileMonitor
{
   vector<TIOContext*> m_cpContexts;
   TIOCallbackProcessingThread* m_pCallbackExecutionThread;
   void ClearContexts ();
   CriticalSection m_CriticalSec;
   TFileMonitor ();
   TFileMonitor ( const string& filename );
   ~TFileMonitor ();
public:
   static TFileMonitor& instance ();
   TIOContext* Watch ( const string& filename );
   void Remove ( TIOContext* ctx );
   void RequestChanges ();
   TIOContext* GetCtx ( size_t index );

};
#endif