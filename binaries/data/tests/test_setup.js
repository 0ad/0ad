// The engine provides:
//   Engine.TS_FAIL = function(msg) { ... }
//
// This file define global helper functions for common test assertions.
// (Functions should be defined with local names too, so they show up properly
// in stack traces.)

function fail(msg)
{
	// Get a list of callers
	let trace = (new Error()).stack.split("\n");
	// Remove the Error ctor and this function from the stack
	trace = trace.splice(2);
	trace = "Stack trace:\n" + trace.join("\n");
	Engine.TS_FAIL(trace + msg);
}

global.TS_FAIL = function(msg)
{
	fail(msg);
};

global.TS_ASSERT = function(e)
{
	if (!e)
		fail("Assert failed");
};

global.TS_ASSERT_EQUALS = function(x, y)
{
	if (x !== y)
		fail("Expected equal, got " + uneval(x) + " !== " + uneval(y));
};

global.TS_ASSERT_EQUALS_APPROX = function(x, y, maxDifference)
{
	TS_ASSERT_NUMBER(maxDifference);

	if (Math.abs(x - y) > maxDifference)
		fail("Expected almost equal, got " + uneval(x) + " !== " + uneval(y));
};

global.TS_ASSERT_UNEVAL_EQUALS = function(x, y)
{
	if (!(uneval(x) === uneval(y)))
		fail("Expected equal, got " + uneval(x) + " !== " + uneval(y));
};

global.TS_ASSERT_EXCEPTION = function(func)
{
	try {
		func();
		Engine.TS_FAIL("Missed exception at:\n" + new Error().stack);
	} catch (e) {
	}
};

global.TS_ASSERT_NUMBER = function(value)
{
	if (typeof value != "number" || !isFinite(value))
		fail("The given value must be a real number!");
};

global.TS_ASSERT_LESS = function(x, y)
{
	if (x >= y)
		fail("Expected less than, got " + uneval(x) + " >= " + uneval(y));
};

global.TS_ASSERT_GREATER = function(x, y)
{
	if (x <= y)
		fail("Expected greater than, got " + uneval(x) + " <= " + uneval(y));
};

global.TS_ASSERT_LESS_EQUAL = function(x, y)
{
	if (x > y)
		fail("Expected less than or equal to, got " + uneval(x) + " > " + uneval(y));
};

global.TS_ASSERT_GREATER_EQUAL = function(x, y)
{
	if (x < y)
		fail("Expected greater than or equal, got " + uneval(x) + " < " + uneval(y));
};
