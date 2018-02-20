/**
 * Marks the affected area with the given tileclass.
 */
function TileClassPainter(tileClass)
{
	this.tileClass = tileClass;
}

TileClassPainter.prototype.paint = function(area)
{
	for (let point of area.points)
		this.tileClass.add(point);
};
