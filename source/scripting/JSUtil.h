// included from SpiderMonkey.h

extern jsval jsu_report_param_error(JSContext* cx, jsval* rval);


// consistent argc checking for normal function wrappers: reports an
// error via JS and returns if number of parameters is incorrect.
// .. require exact number (most common case)
#define JSU_REQUIRE_PARAMS(exact_number)\
	if(argc != exact_number)\
		return jsu_report_param_error(cx, rval);
// .. require 0 params (avoids L4 warning "unused argv param")
#define JSU_REQUIRE_NO_PARAMS()\
	UNUSED2(argv);\
	if(argc != 0)\
		return jsu_report_param_error(cx, rval);
// .. require a certain range (e.g. due to optional params)
#define JSU_REQUIRE_PARAM_RANGE(min_number, max_number)\
	if(!(min_number <= argc && argc <= max_number))\
		return jsu_report_param_error(cx, rval);
// .. require at least a certain count (rarely needed)
#define JSU_REQUIRE_MIN_PARAMS(min_number)\
	if(argc < min_number)\
		return jsu_report_param_error(cx, rval);

// same as JSU_REQUIRE_PARAMS, but used from C++ functions that are
// a bit further removed from SpiderMonkey, i.e. return a
// C++ bool indicating success, and not taking an rval param.
#define JSU_REQUIRE_PARAMS_CPP(exact_number)\
	if(argc != exact_number)\
	{\
		jsu_report_param_error(cx, 0);\
		return false;\
	}


#define JSU_ASSERT(expr, msg)\
STMT(\
	if(!(expr))\
	{\
		JS_ReportError(cx, msg);\
		return JS_FALSE;\
	}\
)
