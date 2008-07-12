// rationale for allocating MemFun_t dynamically:
// need to store class pointer, function, and argument for each registered
// function; single static storage isn't possible. we don't want to break
// C compat in the Loader.h interface, so we can't have it take care of this.
// that leaves dynamic alloc or reserving some static storage freed when
// load registration begins. the former is slower and requires checking
// the thunked function's return value (because we mustn't free MemFun_t
// if the function times out), but is simpler.

// VC7 warns if T::*func is not aligned to its size (4..16 bytes on IA-32).
// this is a bug, since sizeof(void*) would be enough. MS says it won't
// be fixed: see http://www.dotnet247.com/247reference/msgs/1/7782.aspx
// we don't make sure alignment is acceptable because both 12 and 16 bytes
// may be required and padding to LCM(12,16) bytes would be wasteful;
// therefore, just disable the warning.
#if MSC_VERSION
#pragma warning(disable: 4121)
#endif

// does this return code indicate the coroutine yielded and
// wasn't yet finished?
static bool ldr_was_interrupted(int ret)
{
	return (0 < ret && ret <= 100);
}

template<class T> struct MemFun_t : boost::noncopyable
{
	T* const this_;
	int (T::*func)(void);
	MemFun_t(T* this__, int(T::*func_)(void))
		: this_(this__), func(func_) {}
};

template<class T> static int MemFunThunk(void* param, double UNUSED(time_left))
{
	MemFun_t<T>* const mf = (MemFun_t<T>*)param;
	int ret = (mf->this_->*mf->func)();

	if(!ldr_was_interrupted(ret))
		delete mf;
	return ret;
}

template<class T> void RegMemFun(T* this_, int(T::*func)(void),
	const wchar_t* description, int estimated_duration_ms)
{
	void* param = new MemFun_t<T>(this_, func);
	LDR_Register(MemFunThunk<T>, param, description, estimated_duration_ms);
}


////////////////////////////////////////////////////////


template<class T, class Arg> struct MemFun1_t : boost::noncopyable
{
	T* const this_;
	Arg arg;
	int (T::*func)(Arg);
	MemFun1_t(T* this__, int(T::*func_)(Arg), Arg arg_)
		: this_(this__), func(func_), arg(arg_) {}
};

template<class T, class Arg> static int MemFun1Thunk(void* param, double UNUSED(time_left))
{
	MemFun1_t<T, Arg>* const mf = (MemFun1_t<T, Arg>*)param;
	int ret = (mf->this_->*mf->func)(mf->arg);
	if(!ldr_was_interrupted(ret))
		delete mf;
	return ret;
}

template<class T, class Arg> void RegMemFun1(T* this_, int(T::*func)(Arg), Arg arg,
	const wchar_t* description, int estimated_duration_ms)
{
	void* param = new MemFun1_t<T, Arg>(this_, func, arg);
	LDR_Register(MemFun1Thunk<T, Arg>, param, description, estimated_duration_ms);
}
