///////////////////////////////////////////////////////////////////////////
//	Area
//
//	Object representing a group of points/tiles
//
//	points: Array of Point objects
//
///////////////////////////////////////////////////////////////////////////

function Area(points, id)
{
	this.points = (points !== undefined ? points : []);
	this.id = id;
}

Area.prototype.getID = function()
{
	return this.id;
};
