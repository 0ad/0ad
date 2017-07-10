/* Copyright (C) 2013 Wildfire Games.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef INCLUDED_CODE_GENERATION
#define INCLUDED_CODE_GENERATION

/**
 * package code into a single statement.
 *
 * @param STMT_code__ code to be bundled. (must be interpretable as
 * a macro argument, i.e. sequence of tokens).
 * the argument name is chosen to avoid conflicts.
 *
 * notes:
 * - for(;;) { break; } and {} don't work because invocations of macros
 *   implemented with STMT often end with ";", thus breaking if() expressions.
 * - we'd really like to eliminate "conditional expression is constant"
 *   warnings. replacing 0 literals with extern volatile variables fools
 *   VC7 but isn't guaranteed to be free of overhead. we will just
 *   squelch the warning (unfortunately non-portable).
 **/
#define STMT(STMT_code__) do { STMT_code__; } while(false)

/**
 * execute the code passed as a parameter only the first time this is
 * reached.
 * may be called at any time (in particular before main), but is not
 * thread-safe. if that's important, use pthread_once() instead.
 **/
#define ONCE(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
	{\
		ONCE_done__ = true;\
		ONCE_code__;\
	}\
)

/**
 * execute the code passed as a parameter except the first time this is
 * reached.
 * may be called at any time (in particular before main), but is not
 * thread-safe.
 **/
#define ONCE_NOT(ONCE_code__)\
STMT(\
	static bool ONCE_done__ = false;\
	if(!ONCE_done__)\
		ONCE_done__ = true;\
	else\
		ONCE_code__;\
)


/**
 * execute the code passed as a parameter before main is entered.
 *
 * WARNING: if the source file containing this is not directly referenced
 * from anywhere, linkers may discard that object file (unless linking
 * statically). see http://www.cbloom.com/3d/techdocs/more_coding.txt
 **/
#define AT_STARTUP(code__)\
	namespace { struct UID__\
	{\
		UID__() { code__; }\
	} UID2__; }


/**
 * C++ new wrapper: allocates an instance of the given type and stores a
 * pointer to it. sets pointer to 0 on allocation failure.
 *
 * this simplifies application code when on VC6, which may or
 * may not throw/return 0 on failure.
 **/
#define SAFE_NEW(type, ptr)\
	type* ptr;\
	try\
	{\
		ptr = new type();\
	}\
	catch(std::bad_alloc&)\
	{\
		ptr = 0;\
	}

/**
 * delete memory ensuing from new and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_DELETE(p)\
STMT(\
	delete (p);	/* if p == 0, delete is a no-op */ \
	(p) = 0;\
)

/**
 * delete memory ensuing from new[] and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_ARRAY_DELETE(p)\
STMT(\
	delete[] (p);	/* if p == 0, delete is a no-op */ \
	(p) = 0;\
)

/**
 * free memory ensuing from malloc and set the pointer to zero
 * (thus making double-frees safe / a no-op)
 **/
#define SAFE_FREE(p)\
STMT(\
	free((void*)p);	/* if p == 0, free is a no-op */ \
	(p) = 0;\
)

#endif		// #ifndef INCLUDED_CODE_GENERATION
