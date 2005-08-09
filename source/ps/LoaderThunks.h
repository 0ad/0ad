// rationale for allocating MemFun_t dynamically:
// need to store class pointer, function, and argument for each registered
// function; single static storage isn't possible. we don't want to break
// C compat in the Loader.h interface, so we can't have it take care of this.
// that leaves dynamic alloc or reserving some static storage freed when
// load registration begins. the former is slower and requires checking
// the thunked function's return value (because we mustn't free MemFun_t
// if the function times out), but is simpler.

template<class T> struct MemFun_t
{
	T* const this_;
	int(T::*func)(void);
	MemFun_t(T* this__, int(T::*func_)(void))
		: this_(this__), func(func_) {}

	// squelch "unable to generate" warnings
	MemFun_t(const MemFun_t& rhs);
	const MemFun_t& operator=(const MemFun_t& rhs);
};

template<class T> static int MemFunThunk(void* param, double UNUSED(time_left))
{
	MemFun_t<T>* const mf = (MemFun_t<T>*)param;
	int ret = (mf->this_->*mf->func)();
	if(ret <= 0)	// did not time out
		delete mf;
	return ret;
}

template<class T> void RegMemFun(T* this_, int(T::*func)(void),
	const wchar_t* description, int estimated_duration_ms)
{
	void* param = new MemFun_t<T>(this_, func);
	THROW_ERR(LDR_Register(MemFunThunk<T>, param, description, estimated_duration_ms));
}


////////////////////////////////////////////////////////


template<class T, class Arg> struct MemFun1_t
{
	T* const this_;
	Arg arg;
	int(T::*func)(Arg);
	MemFun1_t(T* this__, int(T::*func_)(Arg), Arg arg_)
		: this_(this__), func(func_), arg(arg_) {}

	// squelch "unable to generate" warnings
	MemFun1_t(const MemFun1_t& rhs);
	const MemFun1_t& operator=(const MemFun1_t& rhs);
};

template<class T, class Arg> static int MemFun1Thunk(void* param, double UNUSED(time_left))
{
	MemFun1_t<T, Arg>* const mf = (MemFun1_t<T, Arg>*)param;
	int ret = (mf->this_->*mf->func)(mf->arg);
	if(ret <= 0)	// did not time out
		delete mf;
	return ret;
}

template<class T, class Arg> void RegMemFun1(T* this_, int(T::*func)(Arg), Arg arg,
	const wchar_t* description, int estimated_duration_ms)
{
	void* param = new MemFun1_t<T, Arg>(this_, func, arg);
	THROW_ERR(LDR_Register(MemFun1Thunk<T, Arg>, param, description, estimated_duration_ms));
}
