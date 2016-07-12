#ifndef _FILE_MONITOR_
#define _FILE_MONITOR_
#include <Windows.h>
#include "Threads.h"
#include "Notification.h"
#include <unordered_map>
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
//class TBuffer
//{
//   size_t m_Size;
//   BYTE* m_pData;
//public:
//   TBuffer ( size_t size = g_IoContextBufSize ) :
//      m_Size ( ( !size ) ? g_IoContextBufSize : m_Size )
//   {
//      m_pData = new BYTE [ m_Size ];
//      ::memset ( m_pData , 0 , sizeof ( BYTE )*m_Size );
//   }
//   ~TBuffer ()
//   {
//      delete [ ] m_pData;
//   }
//   const BYTE* buffer () const
//   {
//      return m_pData;
//   }
//   void copy ( BYTE* ptr )
//   {
//      ::memcpy ( m_pData , ptr , sizeof ( BYTE )*g_IoContextBufSize );
//   }
//};
//struct TBufferPool
//{
//   unordered_map<TBuffer*,atomic<bool>> m_pool;
//   TBufferPool ( size_t pool , size_t buffer_size )
//   {
//      for ( size_t i = 0; i < pool; i++ )
//      {
//         m_pool.insert ( pair<TBuffer* , atomic<bool>> ( new TBuffer ( buffer_size ) , true ) );
//      }
//   }
//};
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