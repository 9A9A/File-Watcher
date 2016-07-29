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
   IoContext* pIoCtx = nullptr;
   LPOVERLAPPED pOvlpd;
   DWORD dwBytes = 0;
   while ( m_bThreadRunning && ( WAIT_OBJECT_0 != ::WaitForSingleObject ( m_hCloseEvent , NULL ) ) )
   {
      try
      {
         pCtx = nullptr;
         auto Status = TIOCompletionPortSystem::instance ( ).DequeuePacket ( &dwBytes ,
                                                                            ( PULONG_PTR ) &pCtx ,
                                                                             &pOvlpd ,
                                                                             INFINITE );
         if ( !pCtx && !pOvlpd )
         {
            break;
         }
         pIoCtx = reinterpret_cast< IoContext* >( pCtx );
         auto pIoAsyncOp = ( IoOperation* ) pOvlpd;
         if ( pIoAsyncOp )
         {
            pIoAsyncOp->Lock ( );
            if ( Status.m_bResult != 0 )
            {
               if ( dwBytes )
               {
                  switch ( pIoAsyncOp->m_Code )
                  {
                  case IoOperation::ReadDirectoryChanges:
                     TFileMonitor::IoRequestHandler ( pIoCtx , pIoAsyncOp , dwBytes );
                     break;
                  default:
                     delete pIoAsyncOp;
                     break;
                  }
               }
               else
               {
                  delete pIoAsyncOp;
               }
            }
            else
            {
               delete pIoAsyncOp;
            }
         }
         else
         {
         }
      }
      catch ( runtime_error &err )
      {
         cout << err.what () << endl;
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