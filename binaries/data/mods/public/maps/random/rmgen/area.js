/**
 * @file An Area is a set of points identified by an ID that was registered with the RandomMap.
 */

function Area(points, id)
{
	this.points = points;
	this.id = id;
}

Area.prototype.getID = function()
{
	return this.id;
};
