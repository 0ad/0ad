function VectorDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.sqrt(dx*dx + dz*dz);
}

function SquareVectorDistance(a, b)//A sqrtless vector calculator, to see if that improves speed at all.
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return (dx*dx + dz*dz);
}

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
