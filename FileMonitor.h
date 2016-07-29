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
   struct TIoResult
   {
      int m_bResult;
      DWORD m_dwStatusCode = 0;
   };
   TCompletionPort ( unsigned long NumOfThreads = 0 );
   ~TCompletionPort ( );
   void RegisterHandle ( HANDLE hDevice , ULONG_PTR completionKey );
   bool PostIOPacket ( DWORD numOfBytes , ULONG_PTR dwCompletionKey , LPOVERLAPPED lpOverlapped );
   TCompletionPort::TIoResult DequeuePacket ( LPDWORD numOfBytes , PULONG_PTR dwCompletionKey , OVERLAPPED** overlapped , DWORD waitTime );
   TCompletionPort::TIoResult DequeuePackets ( LPOVERLAPPED_ENTRY lpCompletionPortEntries , ULONG ulCount , PULONG ulNumEntriesRemoved , DWORD dwMilliseconds , BOOL fAlertable );
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
class IoContext;
struct IoOperation;
class TNotification;
class TFileMonitor
{
   vector<IoContext*> m_pContexts;
   CriticalSection m_locker;
public:
   TFileMonitor ( ) = default;
   virtual ~TFileMonitor ( );
   void Watch ( const string& filename );
   static void IoRequestHandler ( IoContext* pCtx , IoOperation* pAsyncOp , size_t dwBytes );
   static void CALLBACK IoRequestProcess ( ULONG_PTR lParam );
   using TOnIncomingRequest = function<void ( IoContext* pMonitor , IoOperation* pAsyncOp , size_t dwBytes )>;
   using TOnAdded = function<void ( TNotification& notify )>;
   using TOnRemoved = function<void ( TNotification& notify )>;
   using TOnModified = function<void ( TNotification& notify )>;
   using TOnRenamedOld = function<void ( TNotification& notify )>;
   using TOnRenamedNew = function<void ( TNotification& notify )>;

   TOnIncomingRequest OnIncomingRequest;
   TOnAdded OnAdded;
   TOnRemoved OnRemoved;
   TOnModified OnModified;
   TOnRenamedNew OnRenamedNew;
   TOnRenamedOld OnRenamedOld;
};
#endif