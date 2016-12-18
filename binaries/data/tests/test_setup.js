// The engine provides:
//   Engine.TS_FAIL = function(msg) { ... }
//
// This file define global helper functions for common test assertions.
// (Functions should be defined with local names too, so they show up properly
// in stack traces.)

function fail(msg)
{
	// Get a list of callers
	let trace = (new Error).stack.split("\n");
	// Remove the Error ctor and this function from the stack
	trace = trace.splice(2);
	trace = "Stack trace:\n" + trace.join("\n");
	Engine.TS_FAIL(trace + msg);
}

global.TS_FAIL = function TS_FAIL(msg)
{
	fail(msg);
}

global.TS_ASSERT = function TS_ASSERT(e)
{
	if (!e)
		fail("Assert failed");
}

global.TS_ASSERT_EQUALS = function TS_ASSERT_EQUALS(x, y)
{
	if (!(x === y))
		fail("Expected equal, got "+uneval(x)+" !== "+uneval(y));
}

global.TS_ASSERT_UNEVAL_EQUALS = function TS_ASSERT_UNEVAL_EQUALS(x, y)
{
	if (!(uneval(x) === uneval(y)))
		fail("Expected equal, got "+uneval(x)+" !== "+uneval(y));
}
