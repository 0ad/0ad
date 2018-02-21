/**
 * The LayeredPainter sets different Terrains within the Area.
 * It choses the Terrain depending on the distance to the border of the Area.
 *
 * The Terrains given in the first array are painted from the border of the area towards the center (outermost first).
 * The widths array has one item less than the Terrains array.
 * Each width specifies how many tiles the corresponding Terrain should be wide (distance to the prior Terrain border).
 * The remaining area is filled with the last terrain.
 */
function LayeredPainter(terrainArray, widths)
{
	if (!(terrainArray instanceof Array))
		throw new Error("LayeredPainter: terrains must be an array!");

	this.terrains = terrainArray.map(terrain => createTerrain(terrain));
	this.widths = widths;
}

LayeredPainter.prototype.paint = function(area)
{
	breadthFirstSearchPaint({
		"area": area,
		"brushSize": 1,
		"gridSize": g_Map.getSize(),
		"withinArea": (areaID, position) => g_Map.area[position.x][position.y] == areaID,
		"paintTile": (point, distance) => {
			let width = 0;
			let i = 0;

			for (; i < this.widths.length; ++i)
			{
				width += this.widths[i];
				if (width >= distance)
					break;
			}

			this.terrains[i].place(point);
		}
	});
};
