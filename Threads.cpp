#include "Threads.h"
#include "Notification.h"

TIOBasicWorkerThread::TIOBasicWorkerThread () :
   m_bThreadRunning ( false ) ,
   m_hCloseEvent ( ::CreateEvent ( NULL , false , false , NULL ) )
{
}
TIOBasicWorkerThread::TIOBasicWorkerThread ( TIOBasicWorkerThread&& rhs )
{
   bool value = rhs.m_bThreadRunning;
   m_bThreadRunning = value;
   m_hCloseEvent = rhs.m_hCloseEvent;
   rhs.m_hCloseEvent = INVALID_HANDLE_VALUE;
   OnThreadExit = rhs.OnThreadExit;
   OnThreadStart = rhs.OnThreadStart;
   m_pBasicThread = std::move ( rhs.m_pBasicThread );
}
TIOBasicWorkerThread::~TIOBasicWorkerThread ()
{
   StopThread ();
   ::CloseHandle ( m_hCloseEvent );
}
TIOBasicWorkerThread& TIOBasicWorkerThread::operator = ( TIOBasicWorkerThread&& rhs )
{
   if ( rhs.m_bThreadRunning )
   {
      rhs.StopThread ();
   }
   bool value = rhs.m_bThreadRunning;
   m_bThreadRunning = value;
   m_hCloseEvent = rhs.m_hCloseEvent;
   rhs.m_hCloseEvent = INVALID_HANDLE_VALUE;
   OnThreadExit = rhs.OnThreadExit;
   OnThreadStart = rhs.OnThreadStart;
   m_pBasicThread = std::move ( rhs.m_pBasicThread );
   return *this;
}
void TIOBasicWorkerThread::SetPriority ( int priority )
{
   if ( m_bThreadRunning )
   {
      ::SetThreadPriority ( handle () , priority );
   }
}
std::thread::native_handle_type TIOBasicWorkerThread::handle ()
{
   if ( m_bThreadRunning && m_pBasicThread )
   {
      return m_pBasicThread->native_handle ();
   }
   return nullptr;
}
HANDLE TIOBasicWorkerThread::CloseEvent ()
{
   return m_hCloseEvent;
}
bool TIOBasicWorkerThread::IsThreadRunning () const
{
   return m_bThreadRunning;
}
void TIOBasicWorkerThread::StartThread ()
{
   if ( !m_bThreadRunning )
   {
      m_bThreadRunning = true;
      ::ResetEvent ( m_hCloseEvent );
      m_pBasicThread = std::make_unique<std::thread> ( &TIOBasicWorkerThread::ThreadLoop , this );
   }
}
void TIOBasicWorkerThread::StopThread ()
{
   if ( m_pBasicThread || m_bThreadRunning )
   {
      m_bThreadRunning = false;
      ::SetEvent ( m_hCloseEvent );
      if ( m_pBasicThread->joinable () )
      {
         m_pBasicThread->join ();
      }
   }
}
void TIOBasicWorkerThread::ThreadLoop ()
{
   if ( OnThreadStart )
   {
      OnThreadStart ( this );
   }
   ThreadRoutine ();
   if ( OnThreadExit )
   {
      OnThreadExit ( this );
   }
   m_bThreadRunning = false;
}
// TIOCallbackProcessingThread
TIOCallbackProcessingThread::TIOCallbackProcessingThread ()
{
}
TIOCallbackProcessingThread::TIOCallbackProcessingThread ( TIOCallbackProcessingThread&& rhs ) : TIOBasicWorkerThread ( std::move ( rhs ) )
{
}
TIOCallbackProcessingThread::~TIOCallbackProcessingThread ()
{
   StopThread ();
}
TIOCallbackProcessingThread& TIOCallbackProcessingThread::operator = ( TIOCallbackProcessingThread&& rhs )
{
   TIOBasicWorkerThread::operator=( std::move ( rhs ) );
   return *this;
}
void TIOCallbackProcessingThread::QueueAPC ( PAPCFUNC func , ULONG_PTR param )
{
   if ( m_bThreadRunning )
   {
      ::QueueUserAPC ( func , handle () , param );
   }
}
void TIOCallbackProcessingThread::ThreadRoutine ()
{
   while ( m_bThreadRunning )
   {
      ::WaitForSingleObjectEx ( m_hCloseEvent , INFINITE , true ); // при получении APC - выполняет их 
   }
}
void CALLBACK TIOCallbackProcessingThread::Cb_ProcessIoContext ( ULONG_PTR lParam )
{
   static size_t index = 0;
   auto thrd_ctx = reinterpret_cast< TIOContext::APCForward* >( lParam );
   FILE_NOTIFY_INFORMATION* pInfo = nullptr;
   DWORD dwOffset = 0;
   do 
   {
      pInfo = reinterpret_cast< FILE_NOTIFY_INFORMATION* >( thrd_ctx->m_pBuf + dwOffset );
      if ( pInfo )
      {
         auto pIoCtx = reinterpret_cast< TIOContext* >( thrd_ctx->m_pIoCtx );
         wstring wFilename ( pInfo->FileName );
         wprintf ( L"%zd " , index++ );
         //wcout << index++ << " ";
         TNotification notify ( pIoCtx , wFilename );
         switch ( pInfo->Action )
         {
            case FILE_ACTION_ADDED:
               notify.SetAction ( FILE_ACTION_ADDED );
               if ( pIoCtx->OnAdded )
               {
                  pIoCtx->OnAdded ( notify );
               }
               break;
            case FILE_ACTION_REMOVED:
               notify.SetAction ( FILE_ACTION_REMOVED );
               if ( pIoCtx->OnRemoved )
               {
                  pIoCtx->OnRemoved ( notify );
               }
               break;
            case FILE_ACTION_MODIFIED:
               notify.SetAction ( FILE_ACTION_MODIFIED );
               if ( pIoCtx->OnModified )
               {
                  pIoCtx->OnModified ( notify );
               }
               break;
            case FILE_ACTION_RENAMED_OLD_NAME:
               notify.SetAction ( FILE_ACTION_RENAMED_OLD_NAME );
               if ( pIoCtx->OnRenamedOld )
               {
                  pIoCtx->OnRenamedOld ( notify );
               }
               break;
            case FILE_ACTION_RENAMED_NEW_NAME:
               notify.SetAction ( FILE_ACTION_RENAMED_NEW_NAME );
               if ( pIoCtx->OnRenamedNew )
               {
                  pIoCtx->OnRenamedNew ( notify );
               }
               break;
         }
         dwOffset += pInfo->NextEntryOffset;
      }
   }
   while ( pInfo->NextEntryOffset != 0 );
   delete [ ] thrd_ctx->m_pBuf;
   delete thrd_ctx;
}
// TIOCompletionPortWorker
TIOCompletionPortWorker::TIOCompletionPortWorker ( TIOCompletionPortWorker&& rhs ) :
   TIOBasicWorkerThread ( std::move ( rhs ) )
{
}
TIOCompletionPortWorker& TIOCompletionPortWorker::operator = ( TIOCompletionPortWorker&& rhs )
{
   TIOBasicWorkerThread::operator=( std::move ( rhs ) );
   return *this;
}
TIOCompletionPortWorker::~TIOCompletionPortWorker ()
{
   StopThread ();
}
void TIOCompletionPortWorker::ThreadRoutine ()
{
   void* pCtx = nullptr;
   LPOVERLAPPED pOverlapped = nullptr;
   TIOContext* pIoCtx = nullptr;
   DWORD dwBytes = 0;
   while ( m_bThreadRunning && ( WAIT_OBJECT_0 != ::WaitForSingleObject ( m_hCloseEvent , NULL ) ) )
   {
      try
      {
         pCtx = nullptr;
         TFileMonitor::instance ().RequestChanges ();
         bool Status_t = TIOCompletionPortSystem::instance ().GetIOPacket ( &dwBytes ,
                                                                            ( PULONG_PTR ) &pCtx ,
                                                                            pOverlapped ,
                                                                            INFINITE );
         if ( !pCtx )
         {
            break;
         }
         pIoCtx = reinterpret_cast< TIOContext* >( pCtx );
         if ( !Status_t || ( Status_t && ( 0 == dwBytes ) ) )
         {
            continue;
         }
         if ( pIoCtx->ExecutionThread () )
         {
            auto thrd_ctx = new TIOContext::APCForward;
            thrd_ctx->m_pBuf = pIoCtx->MakeBufferCopy ();
            thrd_ctx->m_pIoCtx = pIoCtx;
            reinterpret_cast< TIOCallbackProcessingThread* >( pIoCtx->ExecutionThread () )->QueueAPC ( ( PAPCFUNC ) &TIOCallbackProcessingThread::Cb_ProcessIoContext ,
                                                                                                       ( ULONG_PTR ) thrd_ctx );
         }
      }
      catch ( runtime_error &err )
      {
         cout << err.what () << endl;
         break;
      }
      catch ( ... )
      {
         cout << "Nothing to monitor\n";
         break;
      }
   }
}
TIOAsynchronousThreadPool::TIOAsynchronousThreadPool ( size_t NumThreads ) :
   m_nThreads ( NumThreads ) ,
   m_nActiveThreads ( 0 )
{
   m_nThreads = ( !m_nThreads ) ? std::thread::hardware_concurrency () : m_nThreads;
   SpawnAllThreads ();
}
TIOAsynchronousThreadPool::~TIOAsynchronousThreadPool ()
{
   StopAllThreads ();
}
void TIOAsynchronousThreadPool::SetPriority ( size_t index , int priority )
{
   if ( index < m_ThreadPool.size () )
   {
      m_ThreadPool [ index ]->SetPriority ( priority );
   }
}
void TIOAsynchronousThreadPool::SetPriorityAll ( int priority )
{
   for ( auto &i : m_ThreadPool )
   {
      i->SetPriority ( priority );
   }
}
void TIOAsynchronousThreadPool::OnThreadExit ( TIOBasicWorkerThread* )
{
   m_nActiveThreads--;
}
void TIOAsynchronousThreadPool::OnThreadStart ( TIOBasicWorkerThread* )
{
   m_nActiveThreads++;
}
void TIOAsynchronousThreadPool::SpawnAllThreads ()
{
   for ( size_t i = 0; i < m_nThreads; i++ )
   {
      m_ThreadPool.emplace_back ( std::make_unique<TIOCompletionPortWorker> () );
   }
}
size_t TIOAsynchronousThreadPool::GetActiveThreadsNum () const
{
   return m_nActiveThreads;
}
size_t TIOAsynchronousThreadPool::GetTotalThreadsNum () const
{
   return m_nThreads;
}
void TIOAsynchronousThreadPool::StopAllThreads ()
{
   if ( m_nActiveThreads )
   {
      for ( auto&i : m_ThreadPool )
      {
         ::SetEvent ( i->CloseEvent () );
      }//переключить все евенты в signalled
      for ( size_t i = 0; i < m_nActiveThreads; i++ )
      {
         TIOCompletionPortSystem::instance ().PostIOPacket ( 0 , ( DWORD ) NULL , NULL );
      }//разблокировать все потоки ожидающие на GetQueuedCompletionStatus
      for ( auto &i : m_ThreadPool )
      {
         i->StopThread ();
      }//ждать корректного завершения всех потоков
   }
}
void TIOAsynchronousThreadPool::StartAllThreads ()
{
   for ( auto &i : m_ThreadPool )
   {
      i->OnThreadStart = std::bind ( &TIOAsynchronousThreadPool::OnThreadStart , this , std::placeholders::_1 );
      i->OnThreadExit = std::bind ( &TIOAsynchronousThreadPool::OnThreadExit , this , std::placeholders::_1 );
      i->StartThread ();
   }
}