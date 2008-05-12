/**
 * =========================================================================
 * File        : os_cpu.cpp
 * Project     : 0 A.D.
 * Description : OS-specific support functions relating to CPU and memory
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#include "precompiled.h"
#include "os_cpu.h"

ERROR_ASSOCIATE(ERR::OS_CPU_RESTRICTED_AFFINITY, "Cannot set desired CPU affinity", -1);
