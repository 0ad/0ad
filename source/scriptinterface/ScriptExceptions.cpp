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


#include "precompiled.h"

#include "ScriptExceptions.h"

#include "ps/CLogger.h"
#include "scriptinterface/ScriptInterface.h"

bool ScriptException::IsPending(const ScriptRequest& rq)
{
	return JS_IsExceptionPending(rq.cx);
}

bool ScriptException::CatchPending(const ScriptRequest& rq)
{
	if (!JS_IsExceptionPending(rq.cx))
		return false;

	JS::RootedValue excn(rq.cx);
	ENSURE(JS_GetPendingException(rq.cx, &excn));

	if (excn.isUndefined())
	{
		LOGERROR("JavaScript error: (undefined)");
		JS_ClearPendingException(rq.cx);
		return true;
	}

	// As of SM45/52, there is no way to recover a stack in case the thrown thing is not an Error object.
	if (!excn.isObject())
	{
		CStr error;
		ScriptInterface::FromJSVal(rq, excn, error);
		LOGERROR("JavaScript error: %s", error);
		JS_ClearPendingException(rq.cx);
		return true;
	}

	JS::RootedObject excnObj(rq.cx, &excn.toObject());
	JSErrorReport* report = JS_ErrorFromException(rq.cx, excnObj);

	if (!report)
	{
		CStr error;
		ScriptInterface::FromJSVal(rq, excn, error);
		LOGERROR("JavaScript error: %s", error);
		JS_ClearPendingException(rq.cx);
		return true;
	}

	std::stringstream msg;
	msg << (JSREPORT_IS_EXCEPTION(report->flags) ? "JavaScript exception: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}

	// TODO SM52:
	// msg << report->message();

	JS::RootedObject stackObj(rq.cx, ExceptionStackOrNull(rq.cx, excnObj));
	JS::RootedValue stackVal(rq.cx, JS::ObjectOrNullValue(stackObj));
	if (!stackVal.isNull())
	{
		std::string stackText;
		ScriptInterface::FromJSVal(rq, stackVal, stackText);

		std::istringstream stream(stackText);
		for (std::string line; std::getline(stream, line);)
			msg << "\n  " << line;
	}

	LOGERROR("%s", msg.str().c_str());

	// When running under Valgrind, print more information in the error message
	//	VALGRIND_PRINTF_BACKTRACE("->");

	JS_ClearPendingException(rq.cx);
	return true;
}

void ScriptException::ErrorReporter(JSContext* cx, const char* message, JSErrorReport* report)
{
	if (!ScriptInterface::GetScriptInterfaceAndCBData(cx))
	{
		LOGERROR("Javascript - Out of Memory error");
		return;
	}
	ScriptRequest rq(*ScriptInterface::GetScriptInterfaceAndCBData(cx)->pScriptInterface);

	std::stringstream msg;
	bool isWarning = JSREPORT_IS_WARNING(report->flags);
	msg << (isWarning ? "JavaScript warning: " : "JavaScript error: ");
	if (report->filename)
	{
		msg << report->filename;
		msg << " line " << report->lineno << "\n";
	}

	msg << message;

	// If there is an exception, then print its stack trace
	JS::RootedValue excn(rq.cx);
	if (JS_GetPendingException(rq.cx, &excn) && excn.isObject())
	{
		JS::RootedValue stackVal(rq.cx);
		JS::RootedObject excnObj(rq.cx, &excn.toObject());
		JS_GetProperty(rq.cx, excnObj, "stack", &stackVal);

		std::string stackText;
		ScriptInterface::FromJSVal(rq, stackVal, stackText);

		std::istringstream stream(stackText);
		for (std::string line; std::getline(stream, line);)
			msg << "\n  " << line;
	}

	if (isWarning)
		LOGWARNING("%s", msg.str().c_str());
	else
		LOGERROR("%s", msg.str().c_str());

	// When running under Valgrind, print more information in the error message
	//	VALGRIND_PRINTF_BACKTRACE("->");
}

void ScriptException::Raise(const ScriptRequest& rq, const char* format, ...)
{
	va_list ap;
	va_start(ap, format);
	JS_ReportError(rq.cx, format, ap);
	va_end(ap);
}
