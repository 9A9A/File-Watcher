#include "Notification.h"

TNotification::TNotification ( IoContext* ctx , const wstring& filename , DWORD action ) :
   m_pIoContext ( ctx ) ,
   m_Action ( action ) ,
   m_Filename ( filename )
{
}
void TNotification::SetAction ( DWORD action )
{
   m_Action = action;
}
wstring TNotification::Filename ( ) const
{
   return m_Filename;
}
DWORD TNotification::Action ( ) const
{
   return m_Action;
}

const IoContext* TNotification::Context ( ) const
{
   return m_pIoContext;
}

IoOperation::IoOperation ( IoOpCode code , unsigned long buffer_len /*= 64 * KB */ ) :
   m_Code ( code )
{
   hEvent = 0;
   Internal = 0;
   InternalHigh = 0;
   Pointer = 0;
   Offset = 0;
   OffsetHigh = 0;
   m_pBuf = new IOBUF;
   m_pBuf->len = ( !buffer_len ) ? 64 * KB : buffer_len;
   m_pBuf->buf = reinterpret_cast< char* >( calloc ( m_pBuf->len , sizeof ( char ) ) );
}

IoOperation::~IoOperation ( )
{
   Unlock ( );
   delete [ ] m_pBuf->buf;
   delete m_pBuf;
}

IoContext::IoContext ( const string& filename , TFileMonitor* pParent ) :
   m_Filename ( filename ) ,
   m_hFile ( INVALID_HANDLE_VALUE ) ,
   m_pMonitor ( pParent )
{
   if ( ( m_hFile = ::CreateFile ( m_Filename.c_str ( ) ,
                                   FILE_LIST_DIRECTORY ,
                                   FILE_SHARE_READ |
                                   FILE_SHARE_DELETE |
                                   FILE_SHARE_WRITE ,
                                   NULL ,
                                   OPEN_EXISTING ,
                                   FILE_FLAG_OVERLAPPED |
                                   FILE_FLAG_BACKUP_SEMANTICS ,
                                   NULL ) ) == INVALID_HANDLE_VALUE )
   {
      static const string HANDLE_OPEN_ERROR = "Error while opening handle occured : ";
      throw runtime_error ( HANDLE_OPEN_ERROR + to_string ( ::GetLastError ( ) ) );
   }
   TIOCompletionPortSystem::instance ( ).RegisterHandle ( m_hFile , ( ULONG_PTR )this );
}

IoContext::~IoContext ( )
{
   CloseHandle ( m_hFile );
}

HANDLE IoContext::Handle ( ) const
{
   return m_hFile;
}

void IoContext::CheckChanges ( )
{
   auto pOvlpd = new IoOperation ( IoOperation::ReadDirectoryChanges );
   pOvlpd->Lock ( );
   if ( !::ReadDirectoryChangesW ( m_hFile ,
                                   pOvlpd->m_pBuf->buf ,
                                   pOvlpd->m_pBuf->len ,
                                   TRUE ,
                                   FILE_NOTIFY_CHANGE_SECURITY |
                                   FILE_NOTIFY_CHANGE_CREATION |
                                   FILE_NOTIFY_CHANGE_LAST_WRITE |
                                   FILE_NOTIFY_CHANGE_SIZE |
                                   FILE_NOTIFY_CHANGE_ATTRIBUTES |
                                   FILE_NOTIFY_CHANGE_DIR_NAME |
                                   FILE_NOTIFY_CHANGE_FILE_NAME ,
                                   NULL ,
                                   pOvlpd ,
                                   NULL ) )
   {
      static const string READ_DIRECTORY_CHANGES_ERROR = "Error while requesting ReadDirectoryChangesW occured : ";
      delete pOvlpd;
      throw runtime_error ( READ_DIRECTORY_CHANGES_ERROR + to_string ( ::GetLastError ( ) ) );
   }
   pOvlpd->Unlock ( );
}

TFileMonitor* IoContext::Parent ( )
{
   return m_pMonitor;
}
