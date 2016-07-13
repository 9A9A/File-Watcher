#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_
#include <functional>
#include <string>
#include "Threads.h"
#include"FileMonitor.h"
#define KB 1024
using namespace std;
static const size_t g_IoContextBufSize = 128 * KB;
class TNotification;
class TIOContext
{
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
   void CheckChanges ();
   void Lock ();
   void Unlock ();
   string Filename () const;
   BYTE* MakeBufferCopy ();
   virtual ~TIOContext ();
   struct APCForward
   {
      void* m_pIoCtx;
      BYTE* m_pBuf;
   };
   struct IoOperation : public OVERLAPPED
   {
      IoOperation ( size_t optype )
      {
         Internal = 0;
         InternalHigh = 0;
         Offset = 0;
         OffsetHigh = 0;
         Pointer = 0;
         hEvent = 0;
         m_OperationType = optype;
      }
      size_t m_OperationType;
      enum
      {
         Read,
         Write,
         Modified,
         Deleted,
         RenamedOld,
         RenamedNew,
         Accept,
         Close,
         ReadDirectoryChanges
      };
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