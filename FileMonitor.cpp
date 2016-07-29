#include "FileMonitor.h"



// Completion port
TCompletionPort::TCompletionPort ( unsigned long NumOfThreads ) :
   m_hCompletionPort ( ::CreateIoCompletionPort ( INVALID_HANDLE_VALUE , NULL , NULL , NumOfThreads ) )
{
   if ( m_hCompletionPort == 0 )
   {
      throw ::GetLastError ( );
   }
}
TCompletionPort::~TCompletionPort ( )
{
   ::CloseHandle ( m_hCompletionPort );
}
void TCompletionPort::RegisterHandle ( HANDLE handle , ULONG_PTR completionKey )
{
   if ( m_hCompletionPort != ::CreateIoCompletionPort ( handle , m_hCompletionPort , completionKey , NULL ) )
   {
      throw ::GetLastError ( );
   }
}
bool TCompletionPort::PostIOPacket ( DWORD numOfBytes ,
                                     ULONG_PTR dwCompletionKey ,
                                     LPOVERLAPPED lpOverlapped )
{
   if ( NULL == ::PostQueuedCompletionStatus ( m_hCompletionPort ,
                                               numOfBytes ,
                                               dwCompletionKey ,
                                               lpOverlapped ) )
   {
      throw ::GetLastError ( );
   }
   return true;
}

TCompletionPort::TCompletionPort::TIoResult TCompletionPort::DequeuePacket ( LPDWORD numOfBytes , PULONG_PTR dwCompletionKey , OVERLAPPED** overlapped , DWORD waitTime )
{
   TIoResult result;
   result.m_bResult = ::GetQueuedCompletionStatus ( m_hCompletionPort ,
                                                    numOfBytes ,
                                                    dwCompletionKey ,
                                                    overlapped ,
                                                    waitTime );
   result.m_dwStatusCode = ::GetLastError ( );
   return result;
}

TCompletionPort::TCompletionPort::TIoResult TCompletionPort::DequeuePackets ( LPOVERLAPPED_ENTRY lpCompletionPortEntries , ULONG ulCount , PULONG ulNumEntriesRemoved , DWORD dwMilliseconds , BOOL fAlertable )
{
   TIoResult result;
   result.m_bResult = ::GetQueuedCompletionStatusEx ( m_hCompletionPort ,
                                                      lpCompletionPortEntries ,
                                                      ulCount ,
                                                      ulNumEntriesRemoved ,
                                                      dwMilliseconds ,
                                                      fAlertable );
   result.m_dwStatusCode = ::GetLastError ( );
   return result;
}

TIOCompletionPortSystem::TIOCompletionPortSystem ( )
{
   m_pThreadPool = std::make_unique<TIOAsynchronousThreadPool> ( 0 ); // ставим кол-во тредов как процев в системе
   m_pThreadPool->StartAllThreads ( );
}
TIOCompletionPortSystem::~TIOCompletionPortSystem ( )
{
}
TIOCompletionPortSystem& TIOCompletionPortSystem::instance ( )
{
   static TIOCompletionPortSystem globalInstance;
   return globalInstance;
}

TFileMonitor::~TFileMonitor ( )
{
   for ( auto &i : m_pContexts )
   {
      delete i;
   }

}

void TFileMonitor::Watch ( const string& filename )
{
   Locker<CriticalSection> lock ( m_locker );
   auto ctx = new IoContext ( filename , this );
   m_pContexts.push_back ( ctx );
   ctx->CheckChanges ( );
}

void TFileMonitor::IoRequestHandler ( IoContext* pCtx , IoOperation* pAsyncOp , size_t dwBytes )
{
   auto parent = pCtx->Parent ( );
   if ( parent )
   {
      pCtx->CheckChanges ( );
      if ( parent->OnIncomingRequest )
      {
         parent->OnIncomingRequest ( pCtx , pAsyncOp , dwBytes );
      }
   }
   pAsyncOp->Unlock ( );
}

void CALLBACK TFileMonitor::IoRequestProcess ( ULONG_PTR lParam )
{
   auto bypass = ( IoOperation::APCBypass* )lParam;
   DWORD dwOffset = 0;
   FILE_NOTIFY_INFORMATION* pInf = nullptr;
   do 
   {
      pInf = reinterpret_cast< FILE_NOTIFY_INFORMATION* >( bypass->m_IoOp->m_pBuf->buf + dwOffset );
      auto monitor = bypass->m_IoCtx->Parent ( );
      if ( pInf && monitor)
      {
         wstring wfilename ( pInf->FileName );
         TNotification notify ( bypass->m_IoCtx , wfilename );
         switch ( pInf->Action )
         {
         case FILE_ACTION_ADDED:
            if ( monitor->OnAdded )
            {
               monitor->OnAdded ( notify );
            }
            break;
         case FILE_ACTION_REMOVED:
            if ( monitor->OnRemoved )
            {
               monitor->OnRemoved ( notify );
            }
            break;
         case FILE_ACTION_MODIFIED:
            if ( monitor->OnModified )
            {
               monitor->OnModified ( notify );
            }
            break;
         case FILE_ACTION_RENAMED_OLD_NAME:
            if ( monitor->OnRenamedOld )
            {
               monitor->OnRenamedOld ( notify );
            }
            break;
         case FILE_ACTION_RENAMED_NEW_NAME:
            if ( monitor->OnRenamedNew )
            {
               monitor->OnRenamedNew ( notify );
            }
            break;
         }
         dwOffset += pInf->NextEntryOffset;
      }
   }
   while ( pInf->NextEntryOffset != 0 );
   delete bypass->m_IoOp;
   delete bypass;
}