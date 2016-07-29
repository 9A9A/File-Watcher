#ifndef _NOTIFICATION_H_
#define _NOTIFICATION_H_
#include <functional>
#include <string>
#include "Threads.h"
#include"FileMonitor.h"
#define KB 1024
using namespace std;
class TNotification;
struct IOBUF
{
   unsigned long len;
   char* buf;
};
class TFileMonitor;
class IoContext
{
   HANDLE m_hFile;
   TFileMonitor* m_pMonitor;
   string m_Filename;
public:
   IoContext ( const string& filename , TFileMonitor* pParent );
   virtual ~IoContext ( );
   HANDLE Handle ( ) const;
   void CheckChanges ( );
   TFileMonitor* Parent ( );
};
struct IoOperation : public OVERLAPPED
{
   enum IoOpCode
   {
      Read ,
      Write ,
      Modified ,
      Deleted ,
      RenamedOld ,
      RenamedNew ,
      ReadDirectoryChanges
   };
   IoOperation ( IoOpCode code , unsigned long buffer_len = 64 * KB );
   virtual ~IoOperation ( );
   IOBUF* m_pBuf;
   size_t m_Code;
   void Lock ( )
   {
      m_locker.Lock ( );
   }
   void Unlock ( )
   {
      m_locker.Unlock ( );
   }
   struct APCBypass
   {
      IoContext* m_IoCtx;
      IoOperation* m_IoOp;
      size_t m_dwBytes;
   };
private:
   CriticalSection m_locker;
};
class TNotification
{
   IoContext* m_pIoContext;
   wstring m_Filename;
   DWORD m_Action;
   TNotification () = delete;
public:
   TNotification ( IoContext* ctx , const wstring& filename , DWORD action = 0 );
   void SetAction ( DWORD );
   wstring Filename () const;
   DWORD Action () const;
   const IoContext* Context () const;
};
#endif