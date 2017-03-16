//////////////////////////////////////////////////////////////////////
//	Terrain
//
//	Abstract class for terrain placers
//
//////////////////////////////////////////////////////////////////////

function Terrain() {}

Terrain.prototype.place = function(x, z)
{
	// Clear old array
	g_Map.terrainObjects[x][z] = undefined;

	this.placeNew(x, z);
};

Terrain.prototype.placeNew = function() {};

//////////////////////////////////////////////////////////////////////
//	SimpleTerrain
//
//	Class for placing simple terrains
//		(one texture and one tree per tile)
//
//	texture: Terrain texture name
//	treeType: Optional template of the tree entity for this terrain
//
//////////////////////////////////////////////////////////////////////

function SimpleTerrain(texture, treeType)
{
	if (texture === undefined)
		throw("SimpleTerrain: texture not defined");

	this.texture = texture;
	this.treeType = treeType;
}

SimpleTerrain.prototype = new Terrain();
SimpleTerrain.prototype.constructor = SimpleTerrain;
SimpleTerrain.prototype.placeNew = function(x, z)
{
	if (this.treeType !== undefined && g_Map.validT(round(x), round(z), MAP_BORDER_WIDTH))
		g_Map.terrainObjects[x][z] = new Entity(this.treeType, 0, x+0.5, z+0.5, randFloat()*TWO_PI);

	g_Map.texture[x][z] = g_Map.getTextureID(this.texture);
};

//////////////////////////////////////////////////////////////////////
//	RandomTerrain
//
//	Class for placing random SimpleTerrains
//
//	terrains: Array of SimpleTerrain objects
//
//////////////////////////////////////////////////////////////////////

function RandomTerrain(terrains)
{
	if (!(terrains instanceof Array) || !terrains.length)
		throw("RandomTerrain: Invalid terrains array");

	this.terrains = terrains;
}

RandomTerrain.prototype = new Terrain();
RandomTerrain.prototype.constructor = RandomTerrain;
RandomTerrain.prototype.placeNew = function(x, z)
{
	pickRandom(this.terrains).placeNew(x, z);
};
