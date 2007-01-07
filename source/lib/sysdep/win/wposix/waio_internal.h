#ifdef __cplusplus
extern "C" {
#endif

extern _CRTIMP int _open(const char* fn, int mode, ...);
extern _CRTIMP int _read (int fd, void* buf, size_t nbytes);
extern _CRTIMP int _write(int fd, void* buf, size_t nbytes);
extern _CRTIMP int _close(int);

#ifdef __cplusplus
}
#endif
