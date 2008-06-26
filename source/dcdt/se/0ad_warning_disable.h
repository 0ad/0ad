#ifdef _MSC_VER
#pragma warning(push, 3)

// it's way too much work to check all of these (runtime invariants may
// ensure the code is safe but cannot automatically be proved by the
// compiler)
#pragma warning(disable:4701)	// "potentially uninitialized variable"
#endif
