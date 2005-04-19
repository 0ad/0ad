#ifdef __cplusplus
extern "C" {
#endif


const size_t CPU_TYPE_LEN = 49;	// processor brand string is <= 48 chars
extern char cpu_type[CPU_TYPE_LEN];

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

extern void get_cpu_info(void);


#ifdef __cplusplus
}
#endif
