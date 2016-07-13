#include "FileMonitor.h"



// Completion port
TCompletionPort::TCompletionPort ( unsigned long NumOfThreads ) :
   m_hCompletionPort ( ::CreateIoCompletionPort ( INVALID_HANDLE_VALUE , NULL , NULL , NumOfThreads ) )
{
   if ( m_hCompletionPort == 0 )
   {
      throw ::GetLastError ();
   }
}
TCompletionPort::~TCompletionPort ()
{
   ::CloseHandle ( m_hCompletionPort );
}
void TCompletionPort::RegisterHandle ( HANDLE handle , ULONG_PTR completionKey )
{
   if ( m_hCompletionPort != ::CreateIoCompletionPort ( handle , m_hCompletionPort , completionKey , NULL ) )
   {
      throw ::GetLastError ();
   }
}
bool TCompletionPort::GetIOPacket ( TCompletionPort::TIOContext& context )
{
   bool status_t = true;
   if ( NULL == ::GetQueuedCompletionStatus ( m_hCompletionPort ,
        &context.m_TransferredBytes ,
        &context.m_CompletionKey ,
        &context.m_pOverlapped ,
        context.m_WaitDuration )
        )
   {
      DWORD lastError_t = ::GetLastError ();
      if ( lastError_t != WAIT_TIMEOUT )
      {
         throw lastError_t;
      }
      status_t = false;
   }
   return status_t;
}
bool TCompletionPort::GetIOPacket ( LPDWORD numOfBytes ,
                                     PULONG_PTR completionKey ,
                                     OVERLAPPED** overlapped ,
                                     DWORD waitTime )
{
   bool status_t = true;
   if ( NULL == ::GetQueuedCompletionStatus ( m_hCompletionPort ,
        numOfBytes ,
        completionKey ,
        overlapped ,
        waitTime )
        )
   {
      DWORD lastError_t = ::GetLastError ();
      if ( lastError_t != WAIT_TIMEOUT )
      {
         throw lastError_t;
      }
      status_t = false;
   }
   return status_t;
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
      throw ::GetLastError ();
   }
   return true;
}
TIOCompletionPortSystem::TIOCompletionPortSystem ()
{
   m_pThreadPool = std::make_unique<TIOAsynchronousThreadPool> ( 0 ); // ставим кол-во тредов как процев в системе
   m_pThreadPool->StartAllThreads ();
}
TIOCompletionPortSystem::~TIOCompletionPortSystem ()
{
}
TIOCompletionPortSystem& TIOCompletionPortSystem::instance ()
{
   static TIOCompletionPortSystem globalInstance;
   return globalInstance;
}

TFileMonitor::TFileMonitor ()
{
   m_pCallbackExecutionThread = new TIOCallbackProcessingThread;
   m_pCallbackExecutionThread->StartThread ();
}
TFileMonitor::TFileMonitor ( const string& filename ) : TFileMonitor ()
{
   Watch ( filename );
}
TFileMonitor::~TFileMonitor ()
{
   m_pCallbackExecutionThread->StopThread ();
   ClearContexts ();
}
TIOContext* TFileMonitor::Watch ( const string& filename )
{
   Locker<CriticalSection> lock ( m_CriticalSec );
   try
   {
      auto ctx = new TIOContext ( filename );
      ctx->SetExecutionThread ( m_pCallbackExecutionThread );
      m_cpContexts.push_back ( ctx );
      ctx->CheckChanges ();
      return ctx;
   }
   catch ( runtime_error& err )
   {
      cout << err.what ();
      return nullptr;
   }
}
TFileMonitor& TFileMonitor::instance()
{
   static TFileMonitor gInstance;
   return gInstance;
}
void TFileMonitor::ClearContexts ()
{
   Locker<CriticalSection> lock ( m_CriticalSec );
   for ( auto &i : m_cpContexts )
   {
      if ( i )
      {
         delete i;
      }
   }
}
void TFileMonitor::Remove ( TIOContext* ctx )
{
   Locker<CriticalSection> lock ( m_CriticalSec );
   for ( size_t i = 0; i < m_cpContexts.size (); i++ )
   {
      if ( m_cpContexts [ i ] == ctx )
      {
         delete m_cpContexts [ i ]; // deallocate first
         m_cpContexts.erase ( m_cpContexts.begin () + i );
      }
   }
}
TIOContext* TFileMonitor::GetCtx ( size_t index )
{
   Locker<CriticalSection> lock ( m_CriticalSec );
   if ( index < m_cpContexts.size () )
   {
      return m_cpContexts [ index ];
   }
   else
   {
      return nullptr;
   }
}
void TFileMonitor::RequestChanges ()
{ 
//   Locker<CriticalSection> lock ( m_CriticalSec );
   for ( auto &i : m_cpContexts )
   {
      if ( i )
      {
         i->CheckChanges ();
      }
   }
}