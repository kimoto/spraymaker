#include "ut.h"

LPTSTR format_alloc(LPCTSTR format, va_list arg)
{
  int len = ::_vscwprintf(format, arg);
  TCHAR *buffer = new TCHAR[len + 1];
  ::_vsnwprintf_s(buffer, len + 1, _TRUNCATE, format, arg);
  return buffer;
}

void trace(LPCTSTR format, ...)
{
#ifdef _DEBUG
  va_list arg;
  va_start(arg, format);

  LPTSTR buffer = format_alloc(format, arg);
  ::OutputDebugString(buffer);
  delete[] buffer;

  va_end(arg);
#endif
}

UINT vmsgbox(HWND hWnd, LPCTSTR caption, UINT option, LPCTSTR format, va_list arg)
{
  LPTSTR buffer = format_alloc(format, arg);
  UINT ret = ::MessageBox(hWnd, buffer, caption, option);
  delete[] buffer;
  return ret;
}

UINT msgbox(HWND hWnd, LPCTSTR title, UINT option, LPCTSTR format, ...)
{
  va_list arg;
  va_start(arg, format);

  UINT ret = vmsgbox(hWnd, title, option, format, arg);

  va_end(arg);
  return ret;
}

UINT msgbox(LPCTSTR format, ...)
{
  va_list arg;
  va_start(arg, format);

  UINT ret = vmsgbox(NULL, L"debug", MB_OK, format, arg);

  va_end(arg);
  return ret;
}

UINT msgbox(LPCTSTR title, UINT option, LPCTSTR format, ...)
{
  va_list arg;
  va_start(arg, format);

  UINT ret = vmsgbox(NULL, title, option, format, arg);

  va_end(arg);
  return ret;
}

void show_last_error()
{
  LPVOID lpMessageBuffer;
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM,
    NULL,
    GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &lpMessageBuffer,
    0,
    NULL );

  MessageBox(NULL, (LPCWSTR)lpMessageBuffer, TEXT("Error"), MB_OK);
  LocalFree( lpMessageBuffer );
}

LPTSTR wsprintf_alloc(LPCTSTR format, ...)
{
  va_list arg;
  va_start(arg, format);
  LPTSTR buffer = ::format_alloc(format, arg);
  va_end(arg);

  return buffer;
}

char *wc2mbs(LPCTSTR buf)
{
  size_t buf_size = lstrlen(buf) + 1;
  char *new_buf = new char[buf_size];

  size_t writed = 0;
  ::wcstombs_s(&writed, new_buf, buf_size, buf, buf_size);
  if(writed == buf_size){
    return new_buf;
  }else{
    delete [] new_buf;
    return NULL;
  }
}

LPTSTR mbs2wc(const char *buf)
{
  size_t buf_size = strlen(buf) + 1;
  TCHAR *new_buf = new TCHAR[buf_size];

  size_t writed = 0;
  ::mbstowcs_s(&writed, new_buf, buf_size, buf, buf_size);
  if(writed == buf_size){
    return new_buf;
  }else{
    delete[] new_buf;
    return NULL;
  }
}

bool get_file_system_time(LPCTSTR path, SYSTEMTIME *sys_updated_at)
{
  FILETIME updated_at;
  HANDLE hFile = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, NULL);
  if(hFile == INVALID_HANDLE_VALUE){
    ::show_last_error();
    return false;
  }

  ::GetFileTime(hFile, NULL, NULL, &updated_at);

  ::FileTimeToSystemTime(&updated_at, sys_updated_at);
  ::CloseHandle(hFile);
  return true;
}

LPTSTR get_file_system_time_string(LPCTSTR path)
{
  SYSTEMTIME time;
  if( ::get_file_system_time(path, &time) ){
    return ::wsprintf_alloc(L"%02d-%02d-%02d %02d:%02d:%02d",
      time.wYear, time.wMonth, time.wDay,
      time.wHour, time.wMinute, time.wSecond);
  }else{
    ::show_last_error();
    return NULL;
  }
}

void each_directory(LPCTSTR dir, void (*func) (LPCTSTR path), bool recursive_option)
{
  // Is directory?
  if(!::PathIsDirectory(dir)){
    return;
  }

  TCHAR glob_dir[MAX_PATH];
  ::wsprintf(glob_dir, L"%s\\*", dir);

  WIN32_FIND_DATA win32fd = {0};
  HANDLE hFind = ::FindFirstFile(glob_dir, &win32fd);
  if(hFind == INVALID_HANDLE_VALUE){
    return;
  }

  do{
    // do not anything if filename equal "." or ".."
    if(::wcscmp(win32fd.cFileName, L".") == 0 || ::wcscmp(win32fd.cFileName, L"..") == 0){
      ;
    }else{
      TCHAR fullpath[MAX_PATH];
      ::wsprintf(fullpath, L"%s\\%s", dir, win32fd.cFileName);

      if(win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
        if(recursive_option){
          // recursive search
          ::each_directory(fullpath, func, recursive_option);
        }
      }else{
        // call specified callback-function
        func(fullpath);
      }
    }
  }while(::FindNextFile(hFind, &win32fd));
}
