/**
 * A Terrain is a class that modifies an arbitrary property of a given tile.
 */

/**
 * SimpleTerrain paints the given texture on the terrain.
 *
 * Optionally it places an entity on the affected tiles and
 * replaces prior entities added by SimpleTerrain on the same tile.
 */
function SimpleTerrain(texture, templateName = undefined)
{
	if (texture === undefined)
		throw new Error("SimpleTerrain: texture not defined");

	this.texture = texture;
	this.templateName = templateName;
}

SimpleTerrain.prototype.place = function(x, z)
{
	if (g_Map.validT(x, z))
		g_Map.terrainObjects[x][z] = this.templateName ? new Entity(this.templateName, 0, x + 0.5, z + 0.5, randFloat(0, 2 * Math.PI)) : undefined;

	g_Map.texture[x][z] = g_Map.getTextureID(this.texture);
};

/**
 * RandomTerrain places one of the given Terrains on the tile.
 * It choses a random Terrain each tile.
 * This is commonly used to create heterogeneous forests.
 */
function RandomTerrain(terrains)
{
	if (!(terrains instanceof Array) || !terrains.length)
		throw new Error("RandomTerrain: Invalid terrains array");

	this.terrains = terrains;
}

RandomTerrain.prototype.place = function(x, z)
{
	pickRandom(this.terrains).place(x, z);
};
