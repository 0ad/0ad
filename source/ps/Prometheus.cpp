#include "Prometheus.h"

// Globals
int g_xres = 800, g_yres = 600;
int g_bpp  = 32;
int g_freq = 60;

DEFINE_ERROR(PS_OK, "OK");
DEFINE_ERROR(PS_FAIL, "Fail");
