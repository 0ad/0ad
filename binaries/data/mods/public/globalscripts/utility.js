/**
 * returns a clone of a simple object or array
 * Only valid JSON objects are accepted
 * So no recursion, and only plain obects or arrays
 */
function clone(o)
{
	if (o instanceof Array)
		var r = [];
	else if (o instanceof Object)
		var r = {};
	else // native data type
		return o;
	for (var key in o)
		r[key] = clone(o[key]);
	return r;
}

