/**
 * Marks the affected area with the given tileclass.
 */
function TileClassPainter(tileClass)
{
	this.tileClass = tileClass;
}

TileClassPainter.prototype.paint = function(area)
{
	for (let point of area.getPoints())
		this.tileClass.add(point);
};
