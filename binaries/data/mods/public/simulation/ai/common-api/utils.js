function VectorDistance(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.sqrt(dx*dx + dz*dz);
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
