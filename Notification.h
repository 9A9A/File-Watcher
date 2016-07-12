#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_
#include <functional>
#include <string>
#include "Threads.h"
#include"FileMonitor.h"
#define KB 1024
using namespace std;
static const size_t g_IoContextBufSize = 64 * KB;
class TNotification;
class TIOContext
{
   LPOVERLAPPED m_pOverlapped;
   LPBYTE m_pBuffer;
   HANDLE m_hFile;
   string m_Filename;
   TIOBasicWorkerThread* m_pExecutionThread;
   CriticalSection m_locker;
public:
   TIOContext ( const string& filename );
   BYTE* Buffer ();
   void SetExecutionThread ( TIOBasicWorkerThread* pThread );
   HANDLE Handle () const;
   TIOBasicWorkerThread* ExecutionThread ();
   LPOVERLAPPED Overlapped ();
   void CheckChanges ();
   string Filename () const;
   BYTE* MakeBufferCopy ();
   virtual ~TIOContext ();
   struct APCForward
   {
      void* m_pIoCtx;
      BYTE* m_pBuf;
   };

   using CALL_BACK = function<void ( TNotification& )>;
   CALL_BACK OnModified;
   CALL_BACK OnAdded;
   CALL_BACK OnRemoved;
   CALL_BACK OnRenamedOld;
   CALL_BACK OnRenamedNew;
};
class TNotification
{
   TIOContext* m_pIoContext;
   wstring m_Filename;
   DWORD m_Action;
   TNotification () = delete;
public:
   TNotification ( TIOContext* ctx , const wstring& filename , DWORD action = 0 );
   void SetAction ( DWORD );
   wstring Filename () const;
   DWORD Action () const;
   const TIOContext* IoContext () const;
};
#endif