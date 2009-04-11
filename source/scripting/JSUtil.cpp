#include "precompiled.h"

#include "SpiderMonkey.h"

jsval jsu_report_param_error(JSContext* cx, jsval* rval)
{
	JS_ReportError(cx, "Invalid parameter(s) or count");

	if(rval)
		*rval = JSVAL_NULL;

	// yes, we had an error, but returning JS_FALSE would cause SpiderMonkey
	// to abort. that would be hard to debug.
	return JS_TRUE;
}
