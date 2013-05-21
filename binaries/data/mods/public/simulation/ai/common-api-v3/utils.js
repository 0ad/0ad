function VectorDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.sqrt(dx*dx + dz*dz);
}

function SquareVectorDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return (dx*dx + dz*dz);
}
// A is the reference, B must be in "range" of A
// this supposes the range is already squared
function inRange(a, b, range)// checks for X distance
{
	// will avoid unnecessary checking for position in some rare cases... I'm lazy
	if (a === undefined || b === undefined || range === undefined)
		return undefined;
	
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return ((dx*dx + dz*dz ) < range);
}
// slower than SquareVectorDistance, faster than VectorDistance but not exactly accurate.
function ManhattanDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.abs(dx) + Math.abs(dz);
}

function AssocArraytoArray(assocArray) {
	var endArray = [];
	for (var i in assocArray)
		endArray.push(assocArray[i]);
	return endArray;
};

function MemoizeInit(obj)
{
	obj._memoizeCache = {};
}

function Memoize(funcname, func)
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

function ShallowClone(obj)
{
	var ret = {};
	for (var k in obj)
		ret[k] = obj[k];
	return ret;
}

// Picks a random element from an array
function PickRandom(list){
	if (list.length === 0)
	{
		return undefined;
	}
	else
	{
		return list[Math.floor(Math.random()*list.length)];
	}
}
