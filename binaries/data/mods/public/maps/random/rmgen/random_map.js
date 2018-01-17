/**
 * @file The RandomMap stores the elevation grid, terrain textures and entities that are exported to the engine.
 *
 * @param {Number} size - Radius or side length of the map in tiles
 * @param {Number} baseHeight - Initial elevation of the map
 */
function RandomMap(size, baseHeight)
{
	// Size must be 0 to 1024, divisible by patches
	this.size = size;

	// Create 2D arrays for textures, object, and areas
	this.texture = [];
	this.terrainObjects = [];
	this.area = [];

	for (let i = 0; i < size; ++i)
	{
		// Texture IDs
		this.texture[i] = new Uint16Array(size);

		// Area IDs
		this.area[i] = new Uint16Array(size);

		// Entities
		this.terrainObjects[i] = [];
		for (let j = 0; j < size; ++j)
			this.terrainObjects[i][j] = undefined;
	}

	// Create 2D array for heightmap
	let mapSize = size;
	if (!TILE_CENTERED_HEIGHT_MAP)
		++mapSize;

	this.height = [];
	for (let i = 0; i < mapSize; ++i)
	{
		this.height[i] = new Float32Array(mapSize);

		for (let j = 0; j < mapSize; ++j)
			this.height[i][j] = baseHeight;
	}

	// Create name <-> id maps for textures
	this.nameToID = {};
	this.IDToName = [];

	// Array of Entities
	this.objects = [];

	// Array of integers
	this.tileClasses = [];

	this.areaID = 0;

	// Starting entity ID, arbitrary number to leave some space for player entities
	this.entityCount = 150;
}

/**
 * Returns the ID of a texture name.
 * Creates a new ID if there isn't one assigned yet.
 */
RandomMap.prototype.getTextureID = function(texture)
{
	if (texture in this.nameToID)
		return this.nameToID[texture];

	let id = this.IDToName.length;
	this.nameToID[texture] = id;
	this.IDToName[id] = texture;

	return id;
};

/**
 * Returns the next unused entityID.
 */
RandomMap.prototype.getEntityID = function()
{
	return this.entityCount++;
};

/**
 * Determines whether the given coordinates are within the given distance of the passable map area.
 * Should be used to restrict entity placement and path creation.
 */
RandomMap.prototype.validT = function(x, z, distance = 0)
{
	distance += MAP_BORDER_WIDTH;

	if (g_MapSettings.CircularMap)
	{
		let halfSize = Math.floor(this.size / 2);
		return Math.round(Math.euclidDistance2D(x, z, halfSize, halfSize)) < halfSize - distance - 1;
	}
	else
		return x >= distance && z >= distance && x < this.size - distance && z < this.size - distance;
};

/**
 * Determines whether the given coordinates are within the tile grid, passable or not.
 * Should be used to restrict texture painting.
 */
RandomMap.prototype.inMapBounds = function(x, z)
{
	return x >= 0 && z >= 0 && x < this.size && z < this.size;
};

/**
 * Determines whether the given coordinates are within the heightmap grid.
 * Should be used to restrict elevation changes.
 */
RandomMap.prototype.validH = function(x, z)
{
	if (x < 0 || z < 0)
		return false;
	if (TILE_CENTERED_HEIGHT_MAP)
		return x < this.size && z < this.size;
	return x <= this.size && z <= this.size;
};

/**
 * Tests if there is a tileclass with the given ID.
 */
RandomMap.prototype.validClass = function(tileClassID)
{
	return tileClassID >= 0 && tileClassID < this.tileClasses.length;
};

/**
 * Returns the name of the texture of the given tile.
 */
RandomMap.prototype.getTexture = function(x, z)
{
	if (!this.validT(x, z))
		throw new Error("getTexture: invalid tile position (" + x + ", " + z + ")");

	return this.IDToName[this.texture[x][z]];
};

/**
 * Paints the given texture on the given tile.
 */
RandomMap.prototype.setTexture = function(x, z, texture)
{
	if (!this.validT(x, z))
		throw new Error("setTexture: invalid tile position (" + x + ", " + z + ")");

	this.texture[x][z] = this.getTextureID(texture);
};

RandomMap.prototype.getHeight = function(x, z)
{
	if (!this.validH(x, z))
		throw new Error("getHeight: invalid vertex position (" + x + ", " + z + ")");

	return this.height[x][z];
};

RandomMap.prototype.setHeight = function(x, z, height)
{
	if (!this.validH(x, z))
		throw new Error("setHeight: invalid vertex position (" + x + ", " + z + ")");

	this.height[x][z] = height;
};

/**
 * Returns the Entity that was painted by a Terrain class on the given tile or undefined otherwise.
 */
RandomMap.prototype.getTerrainObject = function(x, z)
{
	if (!this.validT(x, z))
		throw new Error("getTerrainObject: invalid tile position (" + x + ", " + z + ")");

	return this.terrainObjects[x][z];
};

/**
 * Places the Entity on the given tile and allows to later replace it if the terrain was painted over.
 */
RandomMap.prototype.setTerrainObject = function(x, z, object)
{
	if (!this.validT(x, z))
		throw new Error("setTerrainObject: invalid tile position (" + x + ", " + z + ")");

	this.terrainObjects[x][z] = object;
};

/**
 * Adds the given Entity to the map at the location it defines.
 */
RandomMap.prototype.addObject = function(obj)
{
	this.objects.push(obj);
};

/**
 * Constructs a new Area object and informs the Map which points correspond to this area.
 */
RandomMap.prototype.createArea = function(points)
{
	let areaID = ++this.areaID;
	for (let p of points)
		this.area[p.x][p.z] = areaID;
	return new Area(points, areaID);
};

/**
 * Returns an unused tileclass ID.
 */
RandomMap.prototype.createTileClass = function()
{
	let newID = this.tileClasses.length;
	this.tileClasses.push(new TileClass(this.size, newID));
	return newID;
};

/**
 * Retrieve interpolated height for arbitrary coordinates within the heightmap grid.
 */
RandomMap.prototype.getExactHeight = function(x, z)
{
	let xi = Math.min(Math.floor(x), this.size);
	let zi = Math.min(Math.floor(z), this.size);
	let xf = x - xi;
	let zf = z - zi;

	let h00 = this.height[xi][zi];
	let h01 = this.height[xi][zi + 1];
	let h10 = this.height[xi + 1][zi];
	let h11 = this.height[xi + 1][zi + 1];

	return (1 - zf) * ((1 - xf) * h00 + xf * h10) + zf * ((1 - xf) * h01 + xf * h11);
};

// Converts from the tile centered height map to the corner based height map, used when TILE_CENTERED_HEIGHT_MAP = true
RandomMap.prototype.cornerHeight = function(x, z)
{
	let count = 0;
	let sumHeight = 0;

	for (let dir of [[-1, -1], [-1, 0], [0, -1], [0, 0]])
		if (this.validH(x + dir[0], z + dir[1]))
		{
			++count;
			sumHeight += this.height[x + dir[0]][z + dir[1]];
		}

	if (count == 0)
		return 0;

	return sumHeight / count;
};

/**
 * Retrieve an array of all Entities placed on the map.
 */
RandomMap.prototype.exportEntityList = function()
{
	// Change rotation from simple 2d to 3d befor giving to engine
	for (let obj of this.objects)
		obj.rotation.y = Math.PI / 2 - obj.rotation.y;

	// Terrain objects e.g. trees
	for (let x = 0; x < this.size; ++x)
		for (let z = 0; z < this.size; ++z)
			if (this.terrainObjects[x][z])
				this.objects.push(this.terrainObjects[x][z]);

	log("Number of entities: " + this.objects.length);
	return this.objects;
};

/**
 * Convert the elevation grid to a one-dimensional array.
 */
RandomMap.prototype.exportHeightData = function()
{
	let heightmapSize = this.size + 1;
	let heightmap = new Uint16Array(Math.square(heightmapSize));

	for (let x = 0; x < heightmapSize; ++x)
		for (let z = 0; z < heightmapSize; ++z)
		{
			let currentHeight = TILE_CENTERED_HEIGHT_MAP ? this.cornerHeight(x, z) : this.height[x][z];

			// Correct height by SEA_LEVEL and prevent under/overflow in terrain data
			heightmap[z * heightmapSize + x] = Math.max(0, Math.min(0xFFFF, Math.floor((currentHeight + SEA_LEVEL) * HEIGHT_UNITS_PER_METRE)));
		}

	return heightmap;
};

/**
 * Assemble terrain textures in a one-dimensional array.
 */
RandomMap.prototype.exportTerrainTextures = function()
{
	let tileIndex = new Uint16Array(Math.square(this.size));
	let tilePriority = new Uint16Array(Math.square(this.size));

	for (let x = 0; x < this.size; ++x)
		for (let z = 0; z < this.size; ++z)
		{
			// TODO: For now just use the texture's index as priority, might want to do this another way
			tileIndex[z * this.size + x] = this.texture[x][z];
			tilePriority[z * this.size + x] = this.texture[x][z];
		}

	return {
		"index": tileIndex,
		"priority": tilePriority
	};
};
