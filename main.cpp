#include "FileMonitor.h"
#include <string>
void OnAdded ( TNotification& notify )
{
   wprintf ( L"[OnAdded] %ls\n" , notify.Filename ().data () );
}
void OnRemoved ( TNotification& notify )
{
   wprintf ( L"[OnRemoved] %ls\n" , notify.Filename ().data () );
}
void OnModified ( TNotification& notify )
{
   wprintf ( L"[OnModified] %ls\n" , notify.Filename ().data() );
}
void OnRenamedOld ( TNotification& notify )
{
   wprintf ( L"[OnRenamedOld] %ls\n" , notify.Filename ().data () );
}
void OnRenamedNew ( TNotification& notify )
{
   wprintf ( L"[OnRenamedNew] %ls\n" , notify.Filename ().data () );
}
int main ()
{
   std::locale current_locale ( "" );
   std::locale::global ( current_locale );
   std::ios::sync_with_stdio ( false );
   auto ctx = TFileMonitor::instance ().Watch ( "C:\\" );
   if ( ctx )
   {
      ctx->OnAdded = bind ( OnAdded , placeholders::_1 );
      ctx->OnRemoved = bind ( OnRemoved , placeholders::_1 );
      ctx->OnModified = bind ( OnModified , placeholders::_1 );
      ctx->OnRenamedOld = bind ( OnRenamedOld , placeholders::_1 );
      ctx->OnRenamedNew = bind ( OnRenamedNew , placeholders::_1 );
   }
   while ( true )
   {
      string x;
      getline ( cin , x );
   }
   system ( "pause" );
}