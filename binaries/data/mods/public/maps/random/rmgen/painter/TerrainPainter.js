/**
 * The TerrainPainter draws a given terrain texture over the given area.
 * When used with TERRAIN_SEPARATOR, an entity is placed on each tile.
 */
function TerrainPainter(terrain)
{
	this.terrain = createTerrain(terrain);
}

TerrainPainter.prototype.paint = function(area)
{
	for (let point of area.getPoints())
		this.terrain.place(point);
};
