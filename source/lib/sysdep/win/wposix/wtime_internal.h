#ifndef INCLUDED_WTIME_INTERNAL
#define INCLUDED_WTIME_INTERNAL

// HACK: on Windows, the HRT makes its final implementation choice
// in the first calibrate call where cpu_freq is available.
// provide a routine that makes the choice when called,
// so app code isn't surprised by a timer change, although the HRT
// does try to keep the timer continuous.
extern void wtime_reset_impl(void);


// convert UTC FILETIME to seconds-since-1970 UTC.
// used by wfilesystem.
#ifndef _FILETIME_	// prevent ICE on VC7
struct _FILETIME;
#endif
extern time_t wtime_utc_filetime_to_time_t(_FILETIME* ft);

#endif	// #ifndef INCLUDED_WTIME_INTERNAL
