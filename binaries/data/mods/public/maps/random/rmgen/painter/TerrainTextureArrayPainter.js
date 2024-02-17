/**
 * Paints the given texture-mapping to the given tiles.
 *
 * @param {string[]} textureIDs - Names of the terrain textures
 * @param {number[]} textureNames - One-dimensional array of indices of texturenames, one for each tile of the entire map.
 * @returns
 */
function TerrainTextureArrayPainter(textureIDs, textureNames)
{
	this.textureIDs = textureIDs;
	this.textureNames = textureNames;
}

TerrainTextureArrayPainter.prototype.paint = function(area)
{
	const sourceSize = Math.sqrt(this.textureIDs.length);
	const scale = sourceSize / g_Map.getSize();

	for (const point of area.getPoints())
	{
		const sourcePos = Vector2D.mult(point, scale).floor();
		g_Map.setTexture(point, this.textureNames[this.textureIDs[sourcePos.x * sourceSize + sourcePos.y]]);
	}
};
