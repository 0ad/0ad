struct DllLoadNotify;

extern void wdll_add_notify(DllLoadNotify*);

struct DllLoadNotify
{
	const char* dll_name;
	int(*func)(void);
	DllLoadNotify* next;

	DllLoadNotify(const char* _dll_name, int(*_func)(void))
	{
		dll_name = _dll_name;
		func = _func;
		wdll_add_notify(this);
	}
};

#define WDLL_LOAD_NOTIFY(dll_name, func)\
	static DllLoadNotify func##_NOTIFY(dll_name, func);
