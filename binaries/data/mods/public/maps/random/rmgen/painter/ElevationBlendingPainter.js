/**
 * The ElevationBlendingPainter sets the elevation of each point of the given area to the weighted targetHeight.
 */
function ElevationBlendingPainter(targetHeight, strength)
{
	this.targetHeight = targetHeight;
	this.strength = strength;
}

ElevationBlendingPainter.prototype.paint = function(area)
{
	for (let point of area.getPoints())
		g_Map.setHeight(point, this.strength * this.targetHeight + (1 - this.strength) * g_Map.getHeight(point));
};
