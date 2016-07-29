#include "FileMonitor.h"
#include <string>
TIOCallbackProcessingThread* cb_thread;
void IncomingRequest ( IoContext* mon , IoOperation* pAsyncOp , size_t dwBytes )
{
   auto bypass = new IoOperation::APCBypass;
   bypass->m_IoCtx = mon;
   bypass->m_IoOp = pAsyncOp;
   bypass->m_dwBytes = dwBytes;
   cb_thread->QueueAPC ( ( PAPCFUNC ) TFileMonitor::IoRequestProcess , ( ULONG_PTR ) bypass );
}

void OnAdded ( TNotification& notify )
{
   //wprintf ( L"[OnAdded] %ls\n" , notify.Filename ().data () );
}
void OnRemoved ( TNotification& notify )
{
   //wprintf ( L"[OnRemoved] %ls\n" , notify.Filename ().data () );
}
void OnModified ( TNotification& notify )
{
   //wprintf ( L"[OnModified] %ls\n" , notify.Filename ().data() );
}
void OnRenamedOld ( TNotification& notify )
{
   //wprintf ( L"[OnRenamedOld] %ls\n" , notify.Filename ().data () );
}
void OnRenamedNew ( TNotification& notify )
{
   //wprintf ( L"[OnRenamedNew] %ls\n" , notify.Filename ().data () );
}
int main ()
{
   std::locale current_locale ( "" );
   std::locale::global ( current_locale );
   std::ios::sync_with_stdio ( false );
   cb_thread = new TIOCallbackProcessingThread;
   cb_thread->StartThread ( );

   auto mon = new TFileMonitor;
   if ( mon )
   {
      mon->OnIncomingRequest = IncomingRequest;
      mon->OnAdded = bind ( OnAdded , placeholders::_1 );
      mon->OnRemoved = bind ( OnRemoved , placeholders::_1 );
      mon->OnModified = bind ( OnModified , placeholders::_1 );
      mon->OnRenamedOld = bind ( OnRenamedOld , placeholders::_1 );
      mon->OnRenamedNew = bind ( OnRenamedNew , placeholders::_1 );
      mon->Watch ( "C:\\" );
   }
   while ( true )
   {
      ::Sleep ( 10000 );
   }
   system ( "pause" );
}