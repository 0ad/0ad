#ifndef __FAKE__WINDOWS_H__
#define __FAKE__WINDOWS_H__

#include <stdio.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int UINT;
typedef long LONG;
typedef unsigned long DWORD, ULONG, LRESULT, LPARAM, WPARAM;

typedef int BOOL;
enum { FALSE, TRUE };

typedef struct _HANDLE {} *HANDLE;
typedef struct _HBRUSH {} *HBRUSH;
typedef struct _HCURSOR {} *HCURSOR;
typedef struct _HEAP {} *HEAP;
typedef struct _HICON {} *HICON;
typedef struct _HINSTANCE {} *HINSTANCE;
typedef struct _HMENU {} *HMENU;
typedef struct _HMODULE {} *HMODULE;
typedef struct _HWND {} *HWND;

enum { INFINITE, CS_HREDRAW, CS_VREDRAW, COLOR_WINDOW, GWL_USERDATA, HWND_TOP, SPI_GETWORKAREA,
       WS_CHILD, WS_VISIBLE, SM_CYCAPTION, SM_CYFRAME, SM_CXSCREEN, SM_CYSCREEN,
       SW_SHOWNORMAL, SW_MINIMIZE, WM_SIZE, WM_SETICON, ICON_BIG, WS_OVERLAPPEDWINDOW,
       WM_CREATE, WM_TIMER, WM_CLOSE, WM_DESTROY, WM_QUIT
     };

typedef void *LPVOID;

#ifdef UNICODE
typedef wchar_t TCHAR;
#define TEXT(x) L##x
#else
typedef char TCHAR;
#define TEXT(x) x
#endif

typedef const TCHAR *LPCTSTR;

typedef char *LPSTR;
typedef const char *LPCSTR;
#define IDI_INFORMATION TEXT("IDI_INFORMATION")
#define IDI_WARNING TEXT("IDI_WARNING")
#define IDI_ERROR TEXT("IDI_ERROR")

typedef void (*LPPROC)(void);

#define WINAPI
#define CALLBACK

struct WNDCLASSEX
{
    int cbSize;
    int style;
    LRESULT CALLBACK(*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HBRUSH hbrBackground;
    LPCTSTR lpszMenuName;
    LPCTSTR lpszClassName;
    HICON hIconSm;
};

struct RECT
{
    LONG left, right, top, bottom;
};

struct MSG
{
};

typedef struct
{
    LPVOID lpCreateParams;
} CREATESTRUCT, *LPCREATESTRUCT;

inline HANDLE CreateEvent(LPVOID, BOOL, BOOL, LPVOID) { return 0; }
inline HANDLE CreateThread(LPVOID, int, DWORD WINAPI(*)(LPVOID), LPVOID, int, LPVOID) { return 0; }
inline int WaitForSingleObject(HANDLE, int) { return 0; }
inline int RegisterClassEx(WNDCLASSEX *) { return 0; }
inline int SetWindowLong(HWND, int, LONG) { return 0; }
inline LPARAM MAKELPARAM(unsigned short, unsigned short) { return 0; }
inline HWND CreateWindow(LPCTSTR, LPVOID, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID) { return 0; }
inline LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LONG GetSystemMetrics(int) { return 0; }
inline int SetWindowPos(HWND, int, LONG, LONG, LONG, LONG, int) { return 0; }
inline int SystemParametersInfo(int, int, LPVOID, int) { return 0; }
inline int ShowWindow(HWND, int) { return 0; }
inline int UpdateWindow(HWND) { return 0; }
inline int SetEvent(HANDLE) { return 0; }
inline BOOL GetMessage(MSG *, LPVOID, int, int) { return FALSE; }
inline int DispatchMessage(MSG *) { return 0; }
inline int GetClientRect(HWND, RECT *) { return 0; }
inline HICON LoadIcon(HINSTANCE, LPCTSTR) { return 0; }
inline unsigned lstrlenA(LPCSTR) { return 0; }
inline int lstrcmpA(LPCSTR, LPCSTR) { return 0; }
inline int lstrcpyA(LPSTR, LPCSTR) { return 0; }
inline int lstrcatA(LPSTR, LPCSTR) { return 0; }
#define wsprintfA sprintf
inline int SetWindowTextA(HWND, LPCSTR) { return 0; }
inline LPVOID HeapAlloc(HEAP, int, ULONG) { return 0; }
inline HEAP GetProcessHeap() { return 0; }
inline int HeapFree(HEAP, int, LPVOID) { return 0; }
inline int DestroyWindow(HWND) { return 0; }
inline LONG GetWindowLong(HWND, int) { return 0; }
inline LRESULT CALLBACK DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline HMODULE LoadLibraryA(LPCSTR) { return 0; }
inline LPPROC GetProcAddress(HMODULE, LPCSTR) { return 0; }
inline int SetTimer(HWND, unsigned, unsigned, unsigned) { return 0; }
inline int KillTimer(HWND, unsigned) { return 0; }
inline DWORD GetTickCount() { return 0; }
inline int ExitProcess(int) { return 0; }
inline bool IsIconic(HWND) { return 0; }
inline HWND GetForegroundWindow() { return 0; }

#endif // __FAKE__WINDOWS_H__
