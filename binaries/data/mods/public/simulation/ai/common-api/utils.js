var API3 = function(m)
{

m.debug = function(output)
{
	if (m.DebugEnabled)
	{
		if (typeof output === "string")
			warn(output);
		else
			warn(uneval(output));
	}
};

m.warn = function(output)
{
	if (typeof output === "string")
		warn("PlayerID " + PlayerID + " |   " + output);
	else
		warn("PlayerID " + PlayerID + " |   " + uneval(output));
};

m.VectorDistance = function(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.sqrt(dx*dx + dz*dz);
}

m.SquareVectorDistance = function(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return (dx*dx + dz*dz);
}
// A is the reference, B must be in "range" of A
// this supposes the range is already squared
m.inRange = function(a, b, range)// checks for X distance
{
	// will avoid unnecessary checking for position in some rare cases... I'm lazy
	if (a === undefined || b === undefined || range === undefined)
		return undefined;
	
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return ((dx*dx + dz*dz ) < range);
}
// slower than SquareVectorDistance, faster than VectorDistance but not exactly accurate.
m.ManhattanDistance = function(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.abs(dx) + Math.abs(dz);
}

m.AssocArraytoArray = function(assocArray) {
	var endArray = [];
	for (var i in assocArray)
		endArray.push(assocArray[i]);
	return endArray;
};

m.MemoizeInit = function(obj)
{
	obj._memoizeCache = {};
}

m.Memoize = function(funcname, func)
{
	return function() {
		var args = funcname + '|' + Array.prototype.join.call(arguments, '|');
		if (args in this._memoizeCache)
			return this._memoizeCache[args];

		var ret = func.apply(this, arguments);
		this._memoizeCache[args] = ret;
		return ret;
	};
}

m.ShallowClone = function(obj)
{
	var ret = {};
	for (var k in obj)
		ret[k] = obj[k];
	return ret;
}

// Picks a random element from an array
m.PickRandom = function(list){
	if (list.length === 0)
		return undefined;
	else
		return list[Math.floor(Math.random()*list.length)];
}

return m;

}(API3);
