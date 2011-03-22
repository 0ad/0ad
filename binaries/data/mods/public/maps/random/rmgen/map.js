//////////////////////////////////////////////////////////////////////
//	Map
//////////////////////////////////////////////////////////////////////

function Map(size, baseHeight)
{
	// Size must be 0 to 1024, divisible by 16
	this.size = size;
	
	// Create 2D arrays for texture, object, and area maps
	this.texture = new Array(size);
	this.terrainObjects = new Array(size);
	this.area = new Array(size);
	
	for (var i = 0; i < size; i++)
	{
		this.texture[i] = new Uint16Array(size);		// uint16
		this.terrainObjects[i] = new Array(size);		// entity
		this.area[i] = new Array(size);					// area
		
		for (var j = 0; j < size; j++)
		{
			this.area[i][j] = {};						// undefined would cause a warning in strict mode
			this.terrainObjects[i][j] = [];
		}
	}
	
	var mapSize = size+1;

	// Create 2D array for heightmap
	this.height = new Array(mapSize);
	for (var i=0; i < mapSize; i++)
	{
		this.height[i] = new Float32Array(mapSize);
		for (var j=0; j < mapSize; j++)
		{	// Initialize height map to baseHeight
			this.height[i][j] = baseHeight;
		}
	}

	// Create name <-> id maps for textures
	this.nameToID = {};
	this.IDToName = [];				//string
	
	// Other arrays
	this.objects = [];				//object
	this.areas = [];					//area
	this.tileClasses = [];				//int
	
	// Starting entity ID
	this.entityCount = 150;
}

Map.prototype.initTerrain = function(baseTerrain)
{
	// Initialize base terrain
	var size = this.size;
	for (var i=0; i < size; i++)
	{
		for (var j=0; j < size; j++)
		{
			baseTerrain.place(i, j);
		}
	}
};

// Return ID of texture (by name)
Map.prototype.getID = function(texture)
{
	if (texture in (this.nameToID))
	{
		return this.nameToID[texture];
	}
	
	// Add new texture
	var id = this.IDToName.length;
	this.nameToID[texture] = id;
	this.IDToName[id] = texture;
	
	return id;
};

// Return next free entity ID
Map.prototype.getEntityID = function()
{
	return this.entityCount++;
}

// Check bounds
Map.prototype.validT = function(x, y)
{
	return x >= 0 && y >= 0 && x < this.size && y < this.size;
};

// Check bounds on height map (size + 1 by size + 1)
Map.prototype.validH = function(x, y)
{
	return x >= 0 && y >= 0 && x <= this.size && y <= this.size;
};

// Check bounds on tile class
Map.prototype.validClass = function(c)
{
	return c >= 0 && c < this.tileClasses.length;
};

Map.prototype.getTexture = function(x, y)
{
	if (!this.validT(x, y))
		error("getTexture: invalid tile position ("+x+", "+y+")");
	
	return this.IDToName[this.texture[x][y]];
};

Map.prototype.setTexture = function(x, y, texture)
{
	if (!this.validT(x, y))
		error("setTexture: invalid tile position ("+x+", "+y+")");
	
	 this.texture[x][y] = this.getID(texture);
};

Map.prototype.getHeight = function(x, y)
{
	if (!this.validH(x, y))
		error("getHeight: invalid vertex position ("+x+", "+y+")");
	
	return this.height[x][y];
};

Map.prototype.setHeight = function(x, y, height)
{
	if (!this.validH(x, y))
		error("setHeight: invalid vertex position ("+x+", "+y+")");
	
	this.height[x][y] = height;
};

Map.prototype.getTerrainObjects = function(x, y)
{
	if (!this.validT(x, y))
		error("getTerrainObjects: invalid tile position ("+x+", "+y+")");

	return this.terrainObjects[x][y];
};

Map.prototype.setTerrainObjects = function(x, y, objects)
{
	if (!this.validT(x, y))
		error("setTerrainObjects: invalid tile position ("+x+", "+y+")");

	this.terrainObjects[x][y] = objects;
};

Map.prototype.placeTerrain = function(x, y, terrain)
{
	terrain.place(x, y);
};

Map.prototype.addObjects = function(obj)
{
	this.objects = this.objects.concat(obj);
};

Map.prototype.createArea = function(placer, painter, constraint)
{
	// Check for multiple painters
	if (painter instanceof Array)
	{
		var painterArray = painter;
		painter = new MultiPainter(painterArray);
	}
	
	// Check for null constraint
	if (constraint === undefined || constraint === null)
	{
		constraint = new NullConstraint();
	}
	else if (constraint instanceof Array)
	{	// Check for multiple constraints
		var constraintArray = constraint;
		constraint = new AndConstraint(constraintArray);
	}
	
	var points = placer.place(constraint);
	if (!points)
		return undefined;
	
	var a = new Area(points);
	for (var i=0; i < points.length; i++)
	{
		this.area[points[i].x][points[i].y] = a;
	}
	
	painter.paint(a);
	this.areas.push(a);
	
	return a;
};

Map.prototype.createObjectGroup = function(placer, player, constraint)
{
	// Check for null constraint
	if (constraint === undefined || constraint === null)
	{
		constraint = new NullConstraint();
	}
	else if (constraint instanceof Array)
	{	// Check for multiple constraints
		var constraintArray = constraint;
		constraint = new AndConstraint(constraintArray);
	}
	
	return placer.place(player, constraint);
};

Map.prototype.createTileClass = function()
{
	this.tileClasses.push(new TileClass(this.size));
	
	return this.tileClasses.length;
};

// Get height taking into account terrain curvature
Map.prototype.getExactHeight = function(x, y)
{
	var xi = min(Math.floor(x), this.size);
	var yi = min(Math.floor(y), this.size);
	var xf = x - xi;
	var yf = y - yi;

	var h00 = this.height[xi][yi];
	var h01 = this.height[xi][yi+1];
	var h10 = this.height[xi+1][yi];
	var h11 = this.height[xi+1][yi+1];

	return ( 1 - yf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + yf * ( ( 1 - xf ) * h01 + xf * h11 ) ;
};

Map.prototype.getMapData = function()
{
	var data = {};
	
	// Build entity array
	var entities = [];
	
	// Terrain objects first (trees)
	var size = this.size;
	for (var x=0; x < size; ++x)
	{
		for (var y=0; y < size; ++y)
		{
			if (this.terrainObjects[x][y].length)
				entities = entities.concat(this.terrainObjects[x][y]);
		}
	}
	
	// Now other entities
	entities = entities.concat(this.objects);
	
	// Convert from tiles to map coordinates
	for (var n in entities)
	{
		var e = entities[n];
		e.x *= 4;
		e.y *= 4;
		
		entities[n] = e;
	}
	data["entities"] = entities;
	
	// Terrain
	data["size"] = this.size;
	
	// Convert 2D heightmap array to flat array
	//	Flat because it's easier to handle by the engine
	var mapSize = size+1;
	var height16 = new Array(mapSize*mapSize);		// uint16
	for (var x=0; x < mapSize; x++)
	{
		for (var y=0; y < mapSize; y++)
		{
			var intHeight = Math.floor((this.height[x][y] + SEA_LEVEL) * 256.0 / 0.35);
			
			if (intHeight > 65000)
				intHeight = 65000;
			else if (intHeight < 0)
				intHeight = 0;
			
			height16[y*mapSize + x] = intHeight;
		}
	}
	data["height"] = height16;
	data["seaLevel"] = SEA_LEVEL;
	
	// Get array of textures used in this map
	var textureNames = [];
	for (var name in this.nameToID)
		textureNames.push(name);
	
	data["textureNames"] = textureNames;
	data["numTextures"] = textureNames.length;
	
	//  Convert 2D tile data to flat array, reodering into patches as expected by MapReader
	var tiles = new Array(size*size);
	var patches = size/16;
	for (var x=0; x < size; x++)
	{
		var patchX = Math.floor(x/16);
		var offX = x%16;
		for (var y=0; y < size; y++)
		{
			var patchY = Math.floor(y/16);
			var offY = y%16;
			tiles[(patchY*patches + patchX)*256 + (offY*16 + offX)] =
				{ 	"texIdx1" : this.texture[x][y],
					"texIdx2" : 0xFFFF,
					"priority" : 0
				};
		}
	}
	data["tileData"] = tiles;
	
	return data;
};
