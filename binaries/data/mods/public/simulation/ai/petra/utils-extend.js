var PETRA = function(m)
{

m.AssocArraytoArray = function(assocArray) {
	var endArray = [];
	for (var i in assocArray)
		endArray.push(assocArray[i]);
	return endArray;
};

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

return m;
}(PETRA);
