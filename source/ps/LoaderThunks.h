
template<class T> struct MemFun_t
{
	T* const this_;
	void(T::*func)(void);
	MemFun_t(T* this__, void(T::*func_)(void))
		: this_(this__), func(func_) {}
};

template<class T> static int MemFunThunk(void* param, double time_left)
{
	MemFun_t<T>* const mf = (MemFun_t<T>*)param;
	(mf->this_->*mf->func)();
	delete mf;
	return 0;
}

template<class T> void RegMemFun(T* this_, void(T::*func)(void),
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
	void(T::*func)(Arg);
	MemFun1_t(T* this__, void(T::*func_)(Arg), Arg arg_)
		: this_(this__), func(func_), arg(arg_) {}
};

template<class T, class Arg> static int MemFun1Thunk(void* param, double time_left)
{
	MemFun1_t<T, Arg>* const mf = (MemFun1_t<T, Arg>*)param;
	(mf->this_->*mf->func)(mf->arg);
	delete mf;
	return 0;
}

template<class T, class Arg> void RegMemFun1(T* this_, void(T::*func)(Arg), Arg arg,
											 const wchar_t* description, int estimated_duration_ms)
{
	void* param = new MemFun1_t<T, Arg>(this_, func, arg);
	THROW_ERR(LDR_Register(MemFun1Thunk<T, Arg>, param, description, estimated_duration_ms));
}