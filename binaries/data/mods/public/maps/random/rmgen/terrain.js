//////////////////////////////////////////////////////////////////////
//	Terrain
//////////////////////////////////////////////////////////////////////

function Terrain() {}
	
Terrain.prototype.place = function(x, y)
{
	// Clear old array
	g_Map.terrainObjects[x][y] = [];
	
	this.placeNew(x, y);
};

Terrain.prototype.placeNew = function() {};

//////////////////////////////////////////////////////////////////////
//	SimpleTerrain
//////////////////////////////////////////////////////////////////////

function SimpleTerrain(texture, treeType)
{
	if (texture === undefined)
		error("SimpleTerrain: texture not defined");
	
	this.texture = texture;
	this.treeType = treeType;
}

SimpleTerrain.prototype = new Terrain();
SimpleTerrain.prototype.constructor = SimpleTerrain;
SimpleTerrain.prototype.placeNew = function(x, y)
{
	if (this.treeType !== undefined)
		g_Map.terrainObjects[x][y].push(new Entity(this.treeType, 0, x+0.5, y+0.5, randFloat()*PI));
	
	g_Map.texture[x][y] = g_Map.getID(this.texture);
};

//////////////////////////////////////////////////////////////////////
//	RandomTerrain
//////////////////////////////////////////////////////////////////////

function RandomTerrain(terrains)
{
	if (!(terrains instanceof Array) || !terrains.length)
		error("Invalid terrains array");
	
	this.terrains = terrains;
}

RandomTerrain.prototype = new Terrain();
RandomTerrain.prototype.constructor = RandomTerrain;
RandomTerrain.prototype.placeNew = function(x, y)
{
	this.terrains[randInt(this.terrains.length)].placeNew(x, y);
};
