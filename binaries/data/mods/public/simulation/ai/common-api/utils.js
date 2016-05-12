var API3 = function(m)
{

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
};

m.SquareVectorDistance = function(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return (dx*dx + dz*dz);
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
};

// slower than SquareVectorDistance, faster than VectorDistance but not exactly accurate.
m.ManhattanDistance = function(a, b)
{
	var dx = a[0] - b[0];
	var dz = a[1] - b[1];
	return Math.abs(dx) + Math.abs(dz);
};

m.AssocArraytoArray = function(assocArray) {
	var endArray = [];
	for (var i in assocArray)
		endArray.push(assocArray[i]);
	return endArray;
};

// Picks a random element from an array
m.PickRandom = function(list)
{
	return list.length ? list[Math.floor(Math.random()*list.length)] : undefined;
};


// Utility functions for conversions of maps of different sizes
// It expects that cell size of map 1 is a multiple of cell size of map 2

// return the index of map2 with max content from indices contained inside the cell i of map1
m.getMaxMapIndex = function(i, map1, map2)
{
	var ratio = map1.cellSize / map2.cellSize;
	var ix = (i % map1.width) * ratio;
	var iy = Math.floor(i / map1.width) * ratio;
	var index;
	for (var kx = 0; kx < ratio; ++kx)
		for (var ky = 0; ky < ratio; ++ky)
			if (!index || map2.map[ix+kx+(iy+ky)*map2.width] > map2.map[index])
				index = ix+kx+(iy+ky)*map2.width;
	return index;
};

// return the list of indices of map2 contained inside the cell i of map1
// map1.cellSize must be a multiple of map2.cellSize
m.getMapIndices = function(i, map1, map2)
{
	var ratio = map1.cellSize / map2.cellSize;	// TODO check that this is integer >= 1 ?
	var ix = (i % map1.width) * ratio;
	var iy = Math.floor(i / map1.width) * ratio;
	var ret = [];
	for (var kx = 0; kx < ratio; ++kx)
		for (var ky = 0; ky < ratio; ++ky)
			ret.push(ix+kx+(iy+ky)*map2.width);
	return ret;
};

// return the list of points of map2 contained inside the cell i of map1 
// map1.cellSize must be a multiple of map2.cellSize
m.getMapPoints = function(i, map1, map2)
{
	var ratio = map1.cellSize / map2.cellSize;	// TODO check that this is integer >= 1 ?
	var ix = (i % map1.width) * ratio;
	var iy = Math.floor(i / map1.width) * ratio;
	var ret = [];
	for (var kx = 0; kx < ratio; ++kx)
		for (var ky = 0; ky < ratio; ++ky)
			ret.push([ix+kx, iy+ky]);
	return ret;
};

return m;

}(API3);
