/**
 * returns a clone of a simple object or array
 * Only valid JSON objects are accepted
 * So no recursion, and only plain objects or arrays
 */
function clone(o)
{
	let r;
	if (o instanceof Array)
		r = [];
	else if (o instanceof Object)
		r = {};
	else // native data type
		return o;
	for (let key in o)
		r[key] = clone(o[key]);
	return r;
}
