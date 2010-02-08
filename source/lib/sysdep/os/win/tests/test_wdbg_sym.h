/* Copyright (c) 2010 Wildfire Games
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

// note: this is more of an on-demand display of the stack trace than
// self-test of it.
// TODO: compare against known-good result?
// problem: results may differ by compiler (e.g. due to differing STL)

#include "lib/self_test.h"

#include <queue>
#include <deque>
#include <list>
#include <map>
#include <stack>

#include "lib/sysdep/os/win/win.h"	// HWND
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/os/win/wdbg_sym.h"
#include "lib/external_libraries/dbghelp.h"


static void* callers[100];
static size_t numCallers;

static LibError OnFrame(const _tagSTACKFRAME64* frame, uintptr_t UNUSED(cbData))
{
	callers[numCallers++] = (void*)frame->AddrPC.Offset;
	return INFO::CB_CONTINUE;
}

// (these must be outside of TestWdbgSym so that we can simply
// search for the function's name as a substring within the ILT
// decorated name (which omits the :: scope resolution operator,
// while debug_ResolveSymbol for the function does not)
__declspec(noinline) static void Func1()
{
	wdbg_sym_WalkStack(OnFrame, 0, 0, L"wdbg_sym_WalkStack");
}

__declspec(noinline) static void Func2()
{
	Func1();
}

__declspec(noinline) static void Func3()
{
	Func2();
}


class TestWdbgSym : public CxxTest::TestSuite 
{
#pragma optimize("", off)

	static void m_test_array()
	{
		struct Small
		{
			int i1;
			int i2;
		};

		struct Large
		{
			double d1;
			double d2;
			double d3;
			double d4;
		};

		Large large_array_of_large_structs[8] = { { 0.0,0.0,0.0,0.0 } }; UNUSED2(large_array_of_large_structs);
		Large small_array_of_large_structs[2] = { { 0.0,0.0,0.0,0.0 } }; UNUSED2(small_array_of_large_structs);
		Small large_array_of_small_structs[8] = { { 1,2 } }; UNUSED2(large_array_of_small_structs);
		Small small_array_of_small_structs[2] = { { 1,2 } }; UNUSED2(small_array_of_small_structs);

		int ints[] = { 1,2,3,4,5 };	UNUSED2(ints);
		wchar_t chars[] = { 'w','c','h','a','r','s',0 }; UNUSED2(chars);

		debug_printf(L"\n(dumping stack frames may result in access violations..)\n");

		// note: we don't want any kind of dialog to be raised, because
		// this test now always runs. therefore, just make sure a decent
		// amount of text (not just "(failed)" error messages) was produced.
		ErrorMessageMem emm = {0};
		const wchar_t* text = debug_BuildErrorMessage(L"dummy", 0,0,0, 0,L"debug_BuildErrorMessage", &emm);
		TS_ASSERT(wcslen(text) > 500);
		{
			std::wofstream s(L"d:\\out.txt");
			s << text;
		}
		debug_FreeErrorMessage(&emm);

		debug_printf(L"(done dumping stack frames)\n");
	}

	// also used by test_stl as an element type
	struct Nested
	{
		int nested_member;
		struct Nested* self_ptr;
	};

	static void m_test_udt()
	{
		Nested nested = { 123 }; nested.self_ptr = &nested;

		typedef struct
		{
			u8 s1;
			u8 s2;
			char s3;
		}
		Small;
		Small small__ = { 0x55, 0xaa, -1 }; UNUSED2(small__);

		struct Large
		{
			u8 large_member_u8;
			std::string large_member_string;
			double large_member_double;
		}
		large = { 0xff, "large struct string", 123456.0 }; UNUSED2(large);


		class Base
		{
			int base_int;
			std::wstring base_wstring;
		public:
			Base()
				: base_int(123), base_wstring(L"base wstring")
			{
			}
		};
		class Derived : private Base
		{
			double derived_double;
		public:
			Derived()
				: derived_double(-1.0)
			{
			}
		}
		derived;

		m_test_array();
	}

	// STL containers and their contents
	static void m_test_stl()
	{
		std::vector<std::wstring> v_wstring;
		v_wstring.push_back(L"ws1"); v_wstring.push_back(L"ws2");

		std::deque<int> d_int;
		d_int.push_back(1); d_int.push_back(2); d_int.push_back(3);
		std::deque<std::string> d_string;
		d_string.push_back("a"); d_string.push_back("b"); d_string.push_back("c");

		std::list<float> l_float;
		l_float.push_back(0.1f); l_float.push_back(0.2f); l_float.push_back(0.3f); l_float.push_back(0.4f); 

		std::map<std::string, int> m_string_int;
		m_string_int.insert(std::make_pair<std::string,int>("s5", 5));
		m_string_int.insert(std::make_pair<std::string,int>("s6", 6));
		m_string_int.insert(std::make_pair<std::string,int>("s7", 7));
		std::map<int, std::string> m_int_string;
		m_int_string.insert(std::make_pair<int,std::string>(1, "s1"));
		m_int_string.insert(std::make_pair<int,std::string>(2, "s2"));
		m_int_string.insert(std::make_pair<int,std::string>(3, "s3"));
		std::map<int, int> m_int_int;
		m_int_int.insert(std::make_pair<int,int>(1, 1));
		m_int_int.insert(std::make_pair<int,int>(2, 2));
		m_int_int.insert(std::make_pair<int,int>(3, 3));

		STL_HASH_MAP<std::string, int> hm_string_int;
		hm_string_int.insert(std::make_pair<std::string,int>("s5", 5));
		hm_string_int.insert(std::make_pair<std::string,int>("s6", 6));
		hm_string_int.insert(std::make_pair<std::string,int>("s7", 7));
		STL_HASH_MAP<int, std::string> hm_int_string;
		hm_int_string.insert(std::make_pair<int,std::string>(1, "s1"));
		hm_int_string.insert(std::make_pair<int,std::string>(2, "s2"));
		hm_int_string.insert(std::make_pair<int,std::string>(3, "s3"));
		STL_HASH_MAP<int, int> hm_int_int;
		hm_int_int.insert(std::make_pair<int,int>(1, 1));
		hm_int_int.insert(std::make_pair<int,int>(2, 2));
		hm_int_int.insert(std::make_pair<int,int>(3, 3));


		std::set<uintptr_t> s_uintptr;
		s_uintptr.insert(0x123); s_uintptr.insert(0x456);

		// empty
		std::deque<u8> d_u8_empty;
		std::list<Nested> l_nested_empty;
		std::map<double,double> m_double_empty;
		std::multimap<int,u8> mm_int_empty;
		std::set<size_t> s_uint_empty;
		std::multiset<char> ms_char_empty;
		std::vector<double> v_double_empty;
		std::queue<double> q_double_empty;
		std::stack<double> st_double_empty;
#if HAVE_STL_HASH
		STL_HASH_MAP<double,double> hm_double_empty;
		STL_HASH_MULTIMAP<double,std::wstring> hmm_double_empty;
		STL_HASH_SET<double> hs_double_empty;
		STL_HASH_MULTISET<double> hms_double_empty;
#endif
#if HAVE_STL_SLIST
		STL_SLIST<double> sl_double_empty;
#endif
		std::string str_empty;
		std::wstring wstr_empty;

		m_test_udt();

		// uninitialized
		std::deque<u8> d_u8_uninit;
		std::list<Nested> l_nested_uninit;
		std::map<double,double> m_double_uninit;
		std::multimap<int,u8> mm_int_uninit;
		std::set<size_t> s_uint_uninit;
		std::multiset<char> ms_char_uninit;
		std::vector<double> v_double_uninit;
		std::queue<double> q_double_uninit;
		std::stack<double> st_double_uninit;
#if HAVE_STL_HASH
		STL_HASH_MAP<double,double> hm_double_uninit;
		STL_HASH_MULTIMAP<double,std::wstring> hmm_double_uninit;
		STL_HASH_SET<double> hs_double_uninit;
		STL_HASH_MULTISET<double> hms_double_uninit;
#endif
#if HAVE_STL_SLIST
		STL_SLIST<double> sl_double_uninit;
#endif
		std::string str_uninit;
		std::wstring wstr_uninit;
	}


	// also exercises all basic types because we need to display some values
	// anyway (to see at a glance whether symbol engine addrs are correct)
	static void m_test_addrs(int p_int, double p_double, char* p_pchar, uintptr_t p_uintptr)
	{
		size_t l_uint = 0x1234;
		bool l_bool = true; UNUSED2(l_bool);
		wchar_t l_wchars[] = L"wchar string";
		enum TestEnum { VAL1=1, VAL2=2 } l_enum = VAL1;
		u8 l_u8s[] = { 1,2,3,4 };
		void (*l_funcptr)(void) = m_test_stl;

		static double s_double = -2.718;
		static char s_chars[] = {'c','h','a','r','s',0};
		static void (*s_funcptr)(int, double, char*, uintptr_t) = m_test_addrs;
		static void* s_ptr = (void*)(uintptr_t)0x87654321;
		static HDC s_hdc = (HDC)0xff0;

#if 0	// output only needed when debugging
		debug_printf(L"\nTEST_ADDRS\n");
		debug_printf(L"p_int     addr=%p val=%d\n", &p_int, p_int);
		debug_printf(L"p_double  addr=%p val=%g\n", &p_double, p_double);
		debug_printf(L"p_pchar   addr=%p val=%hs\n", &p_pchar, p_pchar);
		debug_printf(L"p_uintptr addr=%p val=%lu\n", &p_uintptr, p_uintptr);

		debug_printf(L"l_uint    addr=%p val=%u\n", &l_uint, l_uint);
		debug_printf(L"l_wchars  addr=%p val=%ls\n", &l_wchars, l_wchars);
		debug_printf(L"l_enum    addr=%p val=%d\n", &l_enum, l_enum);
		debug_printf(L"l_u8s     addr=%p val=%d\n", &l_u8s, l_u8s);
		debug_printf(L"l_funcptr addr=%p val=%p\n", &l_funcptr, l_funcptr);
#else
		UNUSED2(p_uintptr);
		UNUSED2(p_pchar);
		UNUSED2(p_double);
		UNUSED2(p_int);
		UNUSED2(l_funcptr);
		UNUSED2(l_enum);
		UNUSED2(l_u8s);
		UNUSED2(l_uint);
		UNUSED2(l_wchars);
#endif

		m_test_stl();

		int uninit_int; UNUSED2(uninit_int);
		float uninit_float; UNUSED2(uninit_float);
		double uninit_double; UNUSED2(uninit_double);
		bool uninit_bool; UNUSED2(uninit_bool);
		HWND uninit_hwnd; UNUSED2(uninit_hwnd);
	}

#pragma optimize("", on)

public:
	void test_stack_trace()
	{
		m_test_addrs(123, 3.1415926535897932384626, "pchar string", 0xf00d);
	}

	void test_stack_walk()
	{
		Func3();
		TS_ASSERT(numCallers >= 3);
		void* funcAddresses[3] = { (void*)&Func1, (void*)&Func2, (void*)&Func3 };
		for(size_t i = 0; i < 3; i++)
		{
			wchar_t func1[DBG_SYMBOL_LEN], func2[DBG_SYMBOL_LEN];
			LibError ret;
			ret = debug_ResolveSymbol(callers[i], func1, 0, 0);
			TS_ASSERT_OK(ret);
			ret = debug_ResolveSymbol(funcAddresses[i], func2, 0, 0);
			TS_ASSERT_OK(ret);
			TS_ASSERT_WSTR_CONTAINS(func2, func1);
		}
	}
};
