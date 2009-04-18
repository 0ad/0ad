#ifndef INCLUDED_WTIME_INTERNAL
#define INCLUDED_WTIME_INTERNAL

// convert UTC FILETIME to seconds-since-1970 UTC.
// used by wfilesystem.
#ifndef _FILETIME_	// prevent ICE on VC7
struct _FILETIME;
#endif
extern time_t wtime_utc_filetime_to_time_t(_FILETIME* ft);

#endif	// #ifndef INCLUDED_WTIME_INTERNAL
