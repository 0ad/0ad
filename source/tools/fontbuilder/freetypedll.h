bool SelectDLL(int DLL);

#define FUNC(ret, name, par) extern ret (* DLL##name) par
#include "freetypedll_funcs.h"
#undef FUNC
