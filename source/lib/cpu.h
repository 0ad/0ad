#ifdef __cplusplus
extern "C" {
#endif


extern char cpu_type[];
extern double cpu_freq;

// -1 if detect not yet called, or cannot be determined
extern int cpus;
extern int cpu_speedstep;
extern int cpu_smp;
	// are there actually multiple physical processors,
	// not only logical hyperthreaded CPUs? relevant for wtime.


// not possible with POSIX calls.
// called from ia32.cpp check_smp
extern int on_each_cpu(void(*cb)());


#ifdef __cplusplus
}
#endif
