// note: this is more of an on-demand display of the stack trace than
// self-test of it.
// TODO: compare against known-good result?
// problem: results may differ by compiler (e.g. due to differing STL)

#include "lib/self_test.h"

#include "lib/sysdep/os/win/win.h"	// HWND
#include "lib/debug.h"	// no wdbg_sym interface needed
#include "lib/sysdep/sysdep.h"
#include "lib/sysdep/os/win/win.h"

#include <queue>
#include <deque>
#include <list>
#include <map>
#include <stack>

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

		// note: prefer simple error (which also generates stack trace) to
		// exception, because it is guaranteed to work (no issues with the
		// debugger swallowing exceptions).
		//DEBUG_DISPLAY_ERROR(L"wdbg_sym self test: check if stack trace below is ok.");
		//RaiseException(0xf001,0,0,0);

		// note: we don't want any kind of dialog to be raised, because
		// this test now always runs. therefore, just make sure a decent
		// amount of text (not just "(failed)" error messages) was produced.
		//
		// however, we can't call debug_dump_stack directly because
		// it'd be reentered if an actual error comes up.
		// therefore, use debug_display_error with DE_HIDE_DIALOG.
		// unfortunately this means we can no longer get at the error text.
		// a sanity check of the text length has been added to debug_display_error
		ErrorMessageMem emm = {0};
		const wchar_t* text = debug_error_message_build(L"dummy", 0,0,0, 0,0, &emm);
		TS_ASSERT(wcslen(text) > 500);
		debug_error_message_free(&emm);
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
		debug_printf("\nTEST_ADDRS\n");

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

		debug_printf("p_int     addr=%p val=%d\n", &p_int, p_int);
		debug_printf("p_double  addr=%p val=%g\n", &p_double, p_double);
		debug_printf("p_pchar   addr=%p val=%s\n", &p_pchar, p_pchar);
		debug_printf("p_uintptr addr=%p val=%lu\n", &p_uintptr, p_uintptr);

		debug_printf("l_uint    addr=%p val=%u\n", &l_uint, l_uint);
		debug_printf("l_wchars  addr=%p val=%ws\n", &l_wchars, l_wchars);
		debug_printf("l_enum    addr=%p val=%d\n", &l_enum, l_enum);
		debug_printf("l_u8s     addr=%p val=%d\n", &l_u8s, l_u8s);
		debug_printf("l_funcptr addr=%p val=%p\n", &l_funcptr, l_funcptr);

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
		// TODO: restore this when it doesn't cause annoying assertion failures
//		m_test_addrs(123, 3.1415926535897932384626, "pchar string", 0xf00d);
	}
};
