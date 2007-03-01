/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	This file was taken off the Protect project on 26-09-2005
*/


#ifndef _EVENT_H_
#define _EVENT_H_

#ifndef _FUNCTOR_H_
#include "FUtils/FUFunctor.h"
#endif // _FUNCTOR_H_

// Zero argument Event
class FUEvent0
{
private:
	typedef IFunctor0<void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	FUEvent0() {}
	~FUEvent0()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	size_t GetHandlerCount() { return handlers.size(); }

	void InsertHandler(Handler* functor)
	{
		handlers.push_back(functor);
	}

	/** This template is used to add generic functors. */
	template <class Class>
	void InsertHandler(Class* handle, void (Class::*_function)())
	{
		handlers.push_back(new FUFunctor0<Class, void>(handle, _function));
	}

	template<class Class>
	void ReleaseHandler(Class* handle, void (Class::*_function)(void))
	{
		void* function = *(void**)&_function;
		HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	void operator()()
	{
		HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			(*(*it))();
		}
	}
};

// One argument Event
template <typename Arg1>
class FUEvent1
{
private:
	typedef IFunctor1<Arg1, void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	FUEvent1() {}

	~FUEvent1()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	size_t GetHandlerCount() { return handlers.size(); }

	void InsertHandler(Handler* functor)
	{
		handlers.push_back(functor);
	}

	/** This template is used to add generic functors. */
	template <class Class, class _A1>
	void InsertHandler(Class* handle, void (Class::*_function)(_A1))
	{
		handlers.push_back(new FUFunctor1<Class, _A1, void>(handle, _function));
	}

	template <class Class, class _A1>
	void ReleaseHandler(Class* handle, void (Class::*_function)(_A1))
	{
		typename HandlerList::iterator it;
		void* function = *(void**)&_function;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
                break;
			}
		}
	}

	void operator()(Arg1 sArgument1)
	{
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			(*(*it))(sArgument1);
		}
	}
};

// Two arguments Event
template <typename Arg1, typename Arg2>
class FUEvent2
{
private:
	typedef IFunctor2<Arg1, Arg2, void> Handler;
	typedef fm::pvector<Handler> HandlerList;
	HandlerList handlers;

public:
	FUEvent2() {}

	~FUEvent2()
	{
		FUAssert(handlers.empty(), CLEAR_POINTER_VECTOR(handlers));
	}

	size_t GetHandlerCount() { return handlers.size(); }

	void InsertHandler(Handler* functor)
	{
		handlers.push_back(functor);
	}

	/** This template is used to add generic functors. */
	template <class Class, class _A1, class _A2>
	void InsertHandler(Class* handle, void (Class::*_function)(_A1, _A2))
	{
		handlers.push_back(new FUFunctor2<Class, _A1, _A2, void>(handle, _function));
	}

	/** This function is used to remove static function callbacks. */
	template <class _A1, class _A2>
	void ReleaseHandler(void (*_function)(_A1, _A2))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(NULL, function))
			{
				delete (*it);
				handlers.erase(it);
				break;
			}
		}
	}

	/** This function is used to remove normal functors. */
	template <class Class, class _A1, class _A2>
	void ReleaseHandler(Class* handle, void (Class::*_function)(_A1, _A2))
	{
		void* function = *(void**)&_function;
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			if ((*it)->Compare(handle, function))
			{
				delete (*it);
				handlers.erase(it);
				break;
			}
		}
	}

	void operator()(Arg1 sArgument1, Arg2 sArgument2)
	{
		typename HandlerList::iterator it;
		for (it = handlers.begin(); it != handlers.end(); ++it)
		{
			(*(*it))(sArgument1, sArgument2);
		}
	}
};

#endif // _EVENT_H_
