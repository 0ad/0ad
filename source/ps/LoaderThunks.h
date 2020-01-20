/* Copyright (C) 2020 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_LOADERTHUNKS
#define INCLUDED_LOADERTHUNKS

// does this return code indicate the coroutine yielded and
// wasn't yet finished?
static inline bool ldr_was_interrupted(int ret)
{
	return (0 < ret && ret <= 100);
}

template<class T> struct MemFun_t
{
	NONCOPYABLE(MemFun_t);
public:
	T* const this_;
	int (T::*func)(void);
	MemFun_t(T* this__, int(T::*func_)(void))
		: this_(this__), func(func_) {}
};

template<class T> static int MemFunThunk(std::shared_ptr<void> param, double UNUSED(time_left))
{
	MemFun_t<T>* const mf = static_cast<MemFun_t<T>*>(param.get());
	return (mf->this_->*mf->func)();
}

template<class T> void RegMemFun(T* this_, int(T::*func)(void),
	const wchar_t* description, int estimated_duration_ms)
{
	LDR_Register(MemFunThunk<T>, std::make_shared<MemFun_t<T>>(this_, func), description, estimated_duration_ms);
}


////////////////////////////////////////////////////////


template<class T, class Arg> struct MemFun1_t
{
	NONCOPYABLE(MemFun1_t);
public:
	T* const this_;
	Arg arg;
	int (T::*func)(Arg);
	MemFun1_t(T* this__, int(T::*func_)(Arg), Arg arg_)
		: this_(this__), func(func_), arg(arg_) {}
};

template<class T, class Arg> static int MemFun1Thunk(shared_ptr<void> param, double UNUSED(time_left))
{
	MemFun1_t<T, Arg>* const mf = static_cast<MemFun1_t<T, Arg>*>(param.get());
	return (mf->this_->*mf->func)(mf->arg);
}

template<class T, class Arg> void RegMemFun1(T* this_, int(T::*func)(Arg), Arg arg,
	const wchar_t* description, int estimated_duration_ms)
{
	LDR_Register(MemFun1Thunk<T, Arg>, std::make_shared<MemFun1_t<T, Arg> >(this_, func, arg), description, estimated_duration_ms);
}

#endif // INCLUDED_LOADERTHUNKS
