#include "posix_types.h"

// atomic "compare and swap". compare the machine word at <location> against
// <expected>; if not equal, return false; otherwise, overwrite it with
// <new_value> and return true.
extern bool CAS_(uintptr_t* location, uintptr_t expected, uintptr_t new_value);

#define CAS(l,o,n) CAS_((uintptr_t*)l, (uintptr_t)o, (uintptr_t)n)