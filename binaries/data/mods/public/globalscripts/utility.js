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

var data = { "sub1": { "sub1prop1":"original" }, "prop1": "original", "prop2": [1,2,3] };
var newData = clone(data);
newData["sub1"]["sub1prop1"] = "modified";

print("data" + uneval(data));
print("newData" + uneval(newData));
