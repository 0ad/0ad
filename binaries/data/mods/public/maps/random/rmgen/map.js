//////////////////////////////////////////////////////////////////////
//	Map
//
//	Class for holding map data and providing basic API to change it
//
//	size: Size of the map in tiles
//	baseHeight: Starting height of the map
//
//////////////////////////////////////////////////////////////////////

function Map(size, baseHeight)
{
	// Size must be 0 to 1024, divisible by patches
	this.size = size;
	
	// Create 2D arrays for textures, object, and areas
	this.texture = new Array(size);
	this.terrainObjects = new Array(size);
	this.area = new Array(size);
	
	for (let i = 0; i < size; ++i)
	{
		this.texture[i] = new Uint16Array(size);	// uint16 - texture IDs
		this.terrainObjects[i] = new Array(size);				// array of entities
		this.area[i] = new Uint16Array(size);		// uint16 - area IDs
		
		for (let j = 0; j < size; ++j)
			this.terrainObjects[i][j] = [];
	}

	// Create 2D array for heightmap
	var mapSize;
	if (TILE_CENTERED_HEIGHT_MAP)
		mapSize = size;
	else
		mapSize = size+1;
	
	this.height = new Array(mapSize);
	for (let i = 0; i < mapSize; ++i)
	{
		this.height[i] = new Float32Array(mapSize);		// float32
		
		for (let j = 0; j < mapSize; ++j)
			this.height[i][j] = baseHeight;
	}

	// Create name <-> id maps for textures
	this.nameToID = {};
	this.IDToName = [];				//string
	
	// Other arrays
	this.objects = [];				//object
	this.tileClasses = [];				//int
	
	this.areaID = 0;
	
	// Starting entity ID
	this.entityCount = 150;
}

Map.prototype.initTerrain = function(baseTerrain)
{
	var size = this.size;

	for (let i = 0; i < size; ++i)
		for (let j = 0; j < size; ++j)
			baseTerrain.place(i, j);
};

// Return ID of texture (by name)
Map.prototype.getTextureID = function(texture)
{
	if (texture in (this.nameToID))
		return this.nameToID[texture];
	
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

// Check bounds on tile map
Map.prototype.validT = function(x, z, distance = 0)
{
	if (g_MapSettings.CircularMap)
	{	// Within map circle
		var halfSize = Math.floor(0.5*this.size);
		var dx = (x - halfSize);
		var dz = (z - halfSize);
		return Math.round(Math.sqrt(dx*dx + dz*dz)) < halfSize - distance - 1;
	}
	else
		// Within map square
		return x >= distance && z >= distance && x < this.size - distance && z < this.size - distance;
};

// Check bounds on tile map
Map.prototype.inMapBounds = function(x, z)
{
	return x >= 0 && z >= 0 && x < this.size && z < this.size;
}

// Check bounds on height map if TILE_CENTERED_HEIGHT_MAP==false then this is (size + 1 by size + 1) otherwise (size, size)
Map.prototype.validH = function(x, z)
{
	if (TILE_CENTERED_HEIGHT_MAP)
		return x >= 0 && z >= 0 && x < this.size && z < this.size;
	else
		return x >= 0 && z >= 0 && x <= this.size && z <= this.size;
};

// Check bounds on tile class
Map.prototype.validClass = function(c)
{
	return c >= 0 && c < this.tileClasses.length;
};

Map.prototype.getTexture = function(x, z)
{
	if (!this.validT(x, z))
		throw("getTexture: invalid tile position ("+x+", "+z+")");
	
	return this.IDToName[this.texture[x][z]];
};

Map.prototype.setTexture = function(x, z, texture)
{
	if (!this.validT(x, z))
		throw("setTexture: invalid tile position ("+x+", "+z+")");
	
	 this.texture[x][z] = this.getTextureID(texture);
};

Map.prototype.getHeight = function(x, z)
{
	if (!this.validH(x, z))
		throw("getHeight: invalid vertex position ("+x+", "+z+")");
	
	return this.height[x][z];
};

Map.prototype.setHeight = function(x, z, height)
{
	if (!this.validH(x, z))
		throw("setHeight: invalid vertex position ("+x+", "+z+")");
	
	this.height[x][z] = height;
};

Map.prototype.getTerrainObjects = function(x, z)
{
	if (!this.validT(x, z))
		throw("getTerrainObjects: invalid tile position ("+x+", "+z+")");

	return this.terrainObjects[x][z];
};

Map.prototype.setTerrainObject = function(x, z, object)
{
	if (!this.validT(x, z, 2))
		throw("setTerrainObject: invalid tile position ("+x+", "+z+")");

	this.terrainObjects[x][z] = object;
};

Map.prototype.placeTerrain = function(x, z, terrain)
{
	terrain.place(x, z);
};

Map.prototype.addObject = function(obj)
{
	this.objects.push(obj);
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
	
	var newID = ++this.areaID;
	var area = new Area(points, newID);
	for (var i=0; i < points.length; i++)
	{
		this.area[points[i].x][points[i].z] = newID;
	}
	
	painter.paint(area);
	
	return area;
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
	var newID = this.tileClasses.length;
	this.tileClasses.push(new TileClass(this.size, newID));
	
	return newID;
};

// Get height taking into account terrain curvature
Map.prototype.getExactHeight = function(x, z)
{
	var xi = min(Math.floor(x), this.size);
	var zi = min(Math.floor(z), this.size);
	var xf = x - xi;
	var zf = z - zi;

	var h00 = this.height[xi][zi];
	var h01 = this.height[xi][zi+1];
	var h10 = this.height[xi+1][zi];
	var h11 = this.height[xi+1][zi+1];

	return ( 1 - zf ) * ( ( 1 - xf ) * h00 + xf * h10 ) + zf * ( ( 1 - xf ) * h01 + xf * h11 ) ;
};

// Converts from the tile centered height map to the corner based height map, used when TILE_CENTERED_HEIGHT_MAP = true
Map.prototype.cornerHeight = function(x, z)
{
	var count = 0;
	var sumHeight = 0;
	
	var dirs = [[-1,-1], [-1,0], [0,-1], [0,0]];
	for each (var dir in dirs)
	{
		if (this.validH(x + dir[0], z + dir[1]))
		{
			count++;
			sumHeight += this.height[x + dir[0]][z + dir[1]];
		}
	}
	
	if (count == 0)
		return 0;
	
	return sumHeight / count;
};

Map.prototype.getFullEntityList = function(rotateForMapExport = false)
{
	// Build entity array
	let entities = [];
	
	// Terrain objects first (trees)
	let size = this.size;
	for (let x = 0; x < size; ++x)
		for (let z = 0; z < size; ++z)
			if (this.terrainObjects[x][z] !== undefined)
				entities.push(this.terrainObjects[x][z]);

	// Now other entities
	for (let i = 0; i < this.objects.length; ++i)
	{
		// Change rotation from simple 2d to 3d befor giving to engine
		if (rotateForMapExport)
			this.objects[i].rotation.y = PI/2 - this.objects[i].rotation.y;
		entities.push(this.objects[i]);
	}
	
	return entities;
};

Map.prototype.getMapData = function()
{
	var data = {};
	
	data.entities = this.getFullEntityList(true);
	
	log("Number of entities: "+ data.entities.length);
	
	// Terrain
	var size = this.size;
	data.size = size;
	
	// Convert 2D heightmap array to flat array
	//	Flat because it's easier to handle by the engine
	var mapSize = size+1;
	var height16 = new Uint16Array(mapSize*mapSize);	// uint16
	for (var x = 0; x < mapSize; x++)
	{
		for (var z = 0; z < mapSize; z++)
		{
			var intHeight;
			if (TILE_CENTERED_HEIGHT_MAP)
				intHeight = Math.floor((this.cornerHeight(x, z) + SEA_LEVEL) * HEIGHT_UNITS_PER_METRE);
			else
				intHeight = Math.floor((this.height[x][z] + SEA_LEVEL) * HEIGHT_UNITS_PER_METRE);
			
			// Prevent under/overflow in terrain data
			if (intHeight > 0xFFFF)
				intHeight = 0xFFFF;
			else if (intHeight < 0)
				intHeight = 0;
			
			height16[z*mapSize + x] = intHeight;
		}
	}
	data.height = height16;
	data.seaLevel = SEA_LEVEL;
	
	// Get array of textures used in this map
	var textureNames = [];
	for (var name in this.nameToID)
		textureNames.push(name);
	data.textureNames = textureNames;
	
	//  Convert 2D tile data to flat array
	var tileIndex = new Uint16Array(size*size);		// uint16
	var tilePriority = new Uint16Array(size*size);	// uint16
	for (let x = 0; x < size; ++x)
	{
		for (let z = 0; z < size; ++z)
		{
			// TODO: For now just use the texture's index as priority, might want to do this another way
			tileIndex[z*size + x] = this.texture[x][z];
			tilePriority[z*size + x] = this.texture[x][z];
		}
	}
	data.tileData = { "index": tileIndex, "priority": tilePriority };
	
	return data;
};
