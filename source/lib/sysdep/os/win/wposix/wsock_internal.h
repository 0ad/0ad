#ifdef __cplusplus
extern "C" {
#endif

extern __declspec(dllimport) int __stdcall WSAStartup(unsigned short, void*);
extern __declspec(dllimport) int __stdcall WSACleanup(void);
extern __declspec(dllimport) int __stdcall WSAAsyncSelect(int s, HANDLE hWnd, unsigned int wMsg, long lEvent);
extern __declspec(dllimport) int __stdcall WSAGetLastError(void);

#ifdef __cplusplus
}
#endif
