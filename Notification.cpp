#include "Notification.h"


TIOContext::TIOContext ( const string& filename ) :
   m_Filename ( filename ) ,
   m_hFile ( INVALID_HANDLE_VALUE )
{
   m_hFile = ::CreateFile ( filename.c_str () ,
                            FILE_LIST_DIRECTORY ,
                            FILE_SHARE_READ |
                            FILE_SHARE_DELETE |
                            FILE_SHARE_WRITE ,
                            NULL ,
                            OPEN_EXISTING ,
                            FILE_FLAG_BACKUP_SEMANTICS |
                            FILE_FLAG_OVERLAPPED ,
                            NULL );
   if ( m_hFile == INVALID_HANDLE_VALUE )
   {
      static const string HANDLE_OPEN_ERROR = "Error while opening handle occured : ";
      throw runtime_error ( HANDLE_OPEN_ERROR + to_string ( ::GetLastError () ) );
   }
   TIOCompletionPortSystem::instance ().RegisterHandle ( m_hFile , ( ULONG_PTR )this );
   m_pOverlapped = new OVERLAPPED;
   ::memset ( m_pOverlapped , 0 , sizeof ( OVERLAPPED ) );
   m_pBuffer = new BYTE [ g_IoContextBufSize ];
   ::memset ( m_pBuffer , 0 , sizeof ( BYTE )*g_IoContextBufSize );
}
void TIOContext::CheckChanges ()
{
   Locker<CriticalSection> lock ( m_locker );
   if ( ReadDirectoryChangesW ( m_hFile ,
        m_pBuffer ,
        g_IoContextBufSize ,
        TRUE ,
        FILE_NOTIFY_CHANGE_SECURITY |
        FILE_NOTIFY_CHANGE_CREATION |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_ATTRIBUTES |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_FILE_NAME ,
        NULL ,
        m_pOverlapped ,
        NULL
        ) )
   {
   }
   else
   {
      static const string READ_DIRECTORY_CHANGES_ERROR = "Error occured ReadDirectoryChangesW : ";
      throw runtime_error ( READ_DIRECTORY_CHANGES_ERROR + to_string ( ::GetLastError () ) );
   }
}
BYTE* TIOContext::Buffer ()
{
   Locker<CriticalSection> lock ( m_locker );
   return m_pBuffer;
}
void TIOContext::SetExecutionThread ( TIOBasicWorkerThread* pThread )
{
   Locker<CriticalSection> lock ( m_locker );
   m_pExecutionThread = pThread;
}
HANDLE TIOContext::Handle () const
{
   return m_hFile;
}
LPOVERLAPPED TIOContext::Overlapped ()
{
   return m_pOverlapped;
}
TIOBasicWorkerThread* TIOContext::ExecutionThread ()
{
   return m_pExecutionThread;
}
string TIOContext::Filename () const
{
   return m_Filename;
}
BYTE* TIOContext::MakeBufferCopy ()
{
   Locker<CriticalSection> lock ( m_locker );
   auto buffer = new BYTE [ g_IoContextBufSize ];
   ::memcpy ( buffer , m_pBuffer , sizeof ( BYTE ) * g_IoContextBufSize );
   return buffer;
}
TIOContext::~TIOContext ()
{
   ::CloseHandle ( m_hFile );
   delete [ ] m_pBuffer;
   delete m_pOverlapped;
}

TNotification::TNotification ( TIOContext* ctx , const wstring& filename , DWORD action ) :
   m_pIoContext ( ctx ) ,
   m_Action ( action ) ,
   m_Filename ( filename )
{
}
void TNotification::SetAction ( DWORD action )
{
   m_Action = action;
}
wstring TNotification::Filename () const
{
   return m_Filename;
}
DWORD TNotification::Action () const
{
   return m_Action;
}
const TIOContext* TNotification::IoContext () const
{
   return m_pIoContext;
}