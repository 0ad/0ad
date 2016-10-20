// TODO: rename/change these functions, so the bounds are more clear

/*
 * Return a random floating point number using Math.random library
 *
 * If no parameter given, the returned float is in the interval [0, 1)
 * If two parameters are given, they are minval and maxval, and the returned float is in the interval [minval, maxval)
 */
function randFloat()
{
	if (arguments.length == 0)
	{
		return Math.random();
	}
	else if (arguments.length == 2)
	{
		var minVal = arguments[0];
		var maxVal = arguments[1];

		return minVal + randFloat() * (maxVal - minVal);
	}
	else
	{
		error("randFloat: invalid number of arguments: "+arguments.length);
		return undefined;
	}
}

/*
 * Return a random integer using Math.random library
 *
 * If one parameter given, it's maxval, and the returned integer is in the interval [0, maxval)
 * If two parameters are given, they are minval and maxval, and the returned integer is in the interval [minval, maxval]
 */
function randInt()
{
	if (arguments.length == 1)
	{
		var maxVal = arguments[0];
		return Math.floor(Math.random() * maxVal);
	}
	else if (arguments.length == 2)
	{
		var minVal = arguments[0];
		var maxVal = arguments[1];

		return minVal + randInt(maxVal - minVal + 1);
	}
	else
	{
		error("randInt: invalid number of arguments: "+arguments.length);
		return undefined;
	}
}
