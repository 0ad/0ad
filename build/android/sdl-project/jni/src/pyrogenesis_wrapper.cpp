#include <android/log.h>

#include <dlfcn.h>

#include "SDL.h" // sets up macro to wrap main()

int main(int argc, char* argv[])
{
	__android_log_print(ANDROID_LOG_INFO, "pyrogenesis", "Started wrapper");

	const char* pyropath = "/data/local/libpyrogenesis_dbg.so";
	__android_log_print(ANDROID_LOG_INFO, "pyrogenesis", "Opening library %s", pyropath);
	void* pyro = dlopen(pyropath, RTLD_NOW);
	if (!pyro)
	{
		__android_log_print(ANDROID_LOG_ERROR, "pyrogenesis", "Failed to open %s (dlerror %s)", pyropath, dlerror());
		return -1;
	}
	__android_log_print(ANDROID_LOG_INFO, "pyrogenesis", "Library opened successfully");

	void* pyromain = dlsym(pyro, "pyrogenesis_main");
	if (!pyromain)
	{
		__android_log_print(ANDROID_LOG_ERROR, "pyrogenesis", "Failed to load entry symbol (dlerror %s)", dlerror());
		return -1;
	}

	__android_log_print(ANDROID_LOG_INFO, "pyrogenesis", "Launching engine code");
	return ((int(*)(int, char*[]))pyromain)(argc, argv);
}
