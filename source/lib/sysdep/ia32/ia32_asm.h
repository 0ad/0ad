#ifndef INCLUDED_IA32_ASM
#define INCLUDED_IA32_ASM

#ifdef __cplusplus
extern "C" {
#endif

extern void ia32_asm_cpuid_init();

// try to call the specified CPUID sub-function. returns true on success or
// false on failure (i.e. CPUID or the specific function not supported).
// returns eax, ebx, ecx, edx registers in above order.
extern bool ia32_asm_cpuid(u32 func, u32* regs);

extern void ia32_asm_atomic_add(intptr_t* location, intptr_t increment);


extern bool ia32_asm_CAS(uintptr_t* location, uintptr_t expected, uintptr_t new_value);

extern uint ia32_asm_control87(uint new_val, uint mask);

extern uint ia32_asm_fpclassify(double d);
extern uint ia32_asm_fpclassifyf(float f);

extern float ia32_asm_rintf(float);
extern double ia32_asm_rint(double);
extern float ia32_asm_fminf(float, float);
extern float ia32_asm_fmaxf(float, float);

extern i32 ia32_asm_i32_from_float(float f);
extern i32 ia32_asm_i32_from_double(double d);
extern i64 ia32_asm_i64_from_double(double d);

extern u64 ia32_asm_rdtsc_edx_eax(void);

// write the current execution state (e.g. all register values) into
// (Win32::CONTEXT*)pcontext (defined as void* to avoid dependency).
extern void ia32_asm_get_current_context(void* pcontext);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef INCLUDED_IA32_ASM
