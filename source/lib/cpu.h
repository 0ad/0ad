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


// set cpu_smp if there's more than 1 physical CPU -
// need to know this for wtime's TSC safety check.
// call on each processor (via on_each_cpu).
extern void cpu_check_smp();


#ifdef __cplusplus
}
#endif
