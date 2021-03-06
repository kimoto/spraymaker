﻿#pragma once

#include <Windows.h>
#include <tchar.h>

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

LPTSTR format_alloc(LPCTSTR format, va_list arg);
void trace(LPCTSTR format, ...);
UINT vmsgbox(HWND hWnd, LPCTSTR title, UINT option, LPCTSTR format, va_list arg);
UINT msgbox(LPCTSTR format, ...);
UINT msgbox(HWND hWnd, LPCTSTR title, UINT option, LPCTSTR format, ...);
UINT msgbox(LPCTSTR title, UINT option, LPCTSTR format, ...);
void show_last_error();
LPTSTR wsprintf_alloc(LPCTSTR format, ...);
char *wc2mbs(LPCTSTR buf);
LPTSTR mbs2wc(const char *buf);
bool get_file_system_time(LPCTSTR path, SYSTEMTIME *sys_updated_at);
LPTSTR get_file_system_time_string(LPCTSTR path);
void each_directory(LPCTSTR dir, void (*func) (LPCTSTR path), bool recursive_option = true);
