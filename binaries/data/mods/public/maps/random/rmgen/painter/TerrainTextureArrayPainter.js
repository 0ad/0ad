/**
 * Paints the given texture-mapping to the given tiles.
 *
 * @param {String[]} textureIDs - Names of the terrain textures
 * @param {Number[]} textureNames - One-dimensional array of indices of texturenames, one for each tile of the entire map.
 * @returns
 */
function TerrainTextureArrayPainter(textureIDs, textureNames)
{
	this.textureIDs = textureIDs;
	this.textureNames = textureNames;
}

TerrainTextureArrayPainter.prototype.paint = function(area)
{
	let sourceSize = Math.sqrt(this.textureIDs.length);
	let scale = sourceSize / g_Map.getSize();

	for (let point of area.points)
	{
		let sourcePos = Vector2D.mult(point, scale).floor();
		g_Map.setTexture(point, this.textureNames[this.textureIDs[sourcePos.x * sourceSize + sourcePos.y]]);
	}
};
