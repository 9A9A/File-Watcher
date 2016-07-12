#ifndef _THREADS_H_
#define _THREADS_H_
#include <thread>
#include <atomic>
#include <memory>
#include <iostream>
#include <vector>
#include <Windows.h>
using namespace std;
#define DisableCopyConstructor(Type) Type(const Type& ) = delete
#define DisableCopyAssignment(Type) Type& operator = (const Type& ) = delete
#define DisableMoveConstructor(Type) Type(Type&& ) = delete
#define DisableMoveAssignment(Type) Type& operator = (Type&& ) = delete
class TIOBasicWorkerThread
{
protected:
   HANDLE m_hCloseEvent;
   std::unique_ptr<std::thread> m_pBasicThread;
   std::atomic<bool> m_bThreadRunning = false;

   virtual void  ThreadLoop ();

   TIOBasicWorkerThread ( const TIOBasicWorkerThread& ) = delete;
   TIOBasicWorkerThread&  operator = ( const TIOBasicWorkerThread& ) = delete;
public:

   TIOBasicWorkerThread ();
   TIOBasicWorkerThread ( TIOBasicWorkerThread&& rhs );
   virtual   ~TIOBasicWorkerThread ();
   void SetPriority ( int priority );

   TIOBasicWorkerThread&  operator = ( TIOBasicWorkerThread&& rhs );

   std::thread::native_handle_type  handle ();
   bool  IsThreadRunning () const;
   HANDLE  CloseEvent ();
   virtual void  StartThread ();
   virtual void  StopThread ();
   virtual void  ThreadRoutine () = 0;

   typedef std::function<void ( TIOBasicWorkerThread* )> TBasicCallback;

   TBasicCallback OnThreadStart;
   TBasicCallback OnThreadExit;
};
class TIOCallbackProcessingThread : public TIOBasicWorkerThread
{
protected:

   TIOCallbackProcessingThread ( const TIOCallbackProcessingThread& ) = delete;
   TIOCallbackProcessingThread&  operator = ( const TIOCallbackProcessingThread& ) = delete;

public:
   TIOCallbackProcessingThread ();
   TIOCallbackProcessingThread ( TIOCallbackProcessingThread&& rhs );
   virtual ~TIOCallbackProcessingThread ();

   TIOCallbackProcessingThread& operator = ( TIOCallbackProcessingThread&& rhs );

   void  QueueAPC ( PAPCFUNC func , ULONG_PTR param );

   virtual void ThreadRoutine ();

   static void CALLBACK Cb_ProcessIoContext ( ULONG_PTR lParam );
};
class TIOCompletionPortWorker : public TIOBasicWorkerThread
{
protected:
   TIOCompletionPortWorker ( const TIOCompletionPortWorker& ) = delete;
   TIOCompletionPortWorker&  operator = ( const TIOCompletionPortWorker& ) = delete;
public:
   TIOCompletionPortWorker () = default;
   TIOCompletionPortWorker ( TIOCompletionPortWorker&& rhs );
   virtual ~TIOCompletionPortWorker ();

   TIOCompletionPortWorker& operator = ( TIOCompletionPortWorker&& rhs );

   virtual void ThreadRoutine ();

};
class TIOAsynchronousThreadPool
{
   std::vector<std::unique_ptr<TIOCompletionPortWorker>> m_ThreadPool;
   size_t m_nThreads;
   std::atomic<size_t> m_nActiveThreads;

   void SpawnAllThreads ();

   void OnThreadExit ( TIOBasicWorkerThread* );
   void OnThreadStart ( TIOBasicWorkerThread* );
public:
   TIOAsynchronousThreadPool ( size_t NumThreads = 1 );
   virtual ~TIOAsynchronousThreadPool ();

   void SetPriorityAll ( int priority );
   void SetPriority ( size_t index , int priority );
   size_t GetActiveThreadsNum () const;
   size_t GetTotalThreadsNum () const;
   void StopAllThreads ();
   void StartAllThreads ();
};
class CriticalSection
{
   CRITICAL_SECTION m_CritSec;
public:
   CriticalSection ()
   {
      ::InitializeCriticalSection ( &m_CritSec );
   }
   ~CriticalSection ()
   {
      ::DeleteCriticalSection ( &m_CritSec );
   }
   void Lock ()
   {
      ::EnterCriticalSection ( &m_CritSec );
   }
   void Unlock ()
   {
      ::LeaveCriticalSection ( &m_CritSec );
   }
};
template<typename LockObject>
class Locker
{
   LockObject* m_locker;
public:
   Locker ( LockObject& obj ) : m_locker ( &obj )
   {
      m_locker->Lock ();
   }
   ~Locker ()
   {
      m_locker->Unlock ();
   }
};
#endif