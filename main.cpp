#include "FileMonitor.h"
#include <string>
void OnAdded ( TNotification& notify )
{
   wcout << "[OnAdded] " << notify.Filename () << endl;
}
void OnRemoved ( TNotification& notify )
{
   wcout << "[OnRemoved] " << notify.Filename () << endl;
}
void OnModified ( TNotification& notify )
{
   wcout << "[OnModified] " << notify.Filename () << endl;
}
void OnRenamedOld ( TNotification& notify )
{
   wcout << "[OnRenamedOld] " << notify.Filename () << endl;
}
void OnRenamedNew ( TNotification& notify )
{
   wcout << "[OnRenamedNew] " << notify.Filename () << endl;
}
int main ()
{
   std::locale current_locale ( "" );
   std::locale::global ( current_locale );
   std::ios::sync_with_stdio ( false );
   auto ctx = TFileMonitor::instance ().Watch ( "D:\\Sources" );
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