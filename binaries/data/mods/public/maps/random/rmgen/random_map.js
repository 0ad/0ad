/**
 * @file The RandomMap stores the elevation grid, terrain textures and entities that are exported to the engine.
 *
 * @param {Number} baseHeight - Initial elevation of the map
 * @param {String|Array} baseTerrain - One or more texture names
 */
function RandomMap(baseHeight, baseTerrain)
{
	this.logger = new RandomMapLogger();

	// Size must be 0 to 1024, divisible by patches
	this.size = g_MapSettings.Size;

	// Create name <-> id maps for textures
	this.nameToID = {};
	this.IDToName = [];

	// Texture 2D array
	this.texture = [];
	for (let x = 0; x < this.size; ++x)
	{
		this.texture[x] = new Uint16Array(this.size);

		for (let z = 0; z < this.size; ++z)
			this.texture[x][z] = this.getTextureID(
				typeof baseTerrain == "string" ? baseTerrain : pickRandom(baseTerrain));
	}

	// Create 2D arrays for terrain objects and areas
	this.terrainEntities = [];
	this.area = [];

	for (let i = 0; i < this.size; ++i)
	{
		this.area[i] = new Uint16Array(this.size);

		this.terrainEntities[i] = [];
		for (let j = 0; j < this.size; ++j)
			this.terrainEntities[i][j] = undefined;
	}

	// Create 2D array for heightmap
	let mapSize = this.size;
	if (!TILE_CENTERED_HEIGHT_MAP)
		++mapSize;

	this.height = [];
	for (let i = 0; i < mapSize; ++i)
	{
		this.height[i] = new Float32Array(mapSize);

		for (let j = 0; j < mapSize; ++j)
			this.height[i][j] = baseHeight;
	}

	this.entities = [];

	this.areaID = 0;

	// Starting entity ID, arbitrary number to leave some space for player entities
	this.entityCount = 150;
}

RandomMap.prototype.log = function(text)
{
	this.logger.print(text);
};

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

RandomMap.prototype.isCircularMap = function()
{
	return !!g_MapSettings.CircularMap;
};

RandomMap.prototype.getSize = function()
{
	return this.size;
};

/**
 * Returns the center tile coordinates of the map.
 */
RandomMap.prototype.getCenter = function()
{
	return deepfreeze(new Vector2D(this.size / 2, this.size / 2));
};

/**
 * Returns a human-readable reference to the smallest and greatest coordinates of the map.
 */
RandomMap.prototype.getBounds = function()
{
	return deepfreeze({
		"left": 0,
		"right": this.size,
		"top": this.size,
		"bottom": 0
	});
};

/**
 * Determines whether the given coordinates are within the given distance of the map area.
 * Should be used to restrict terrain texture changes and actor placement.
 * Entity placement should be checked against validTilePassable to exclude the map border.
 */
RandomMap.prototype.validTile = function(position, distance = 0)
{
	if (this.isCircularMap())
		return Math.round(position.distanceTo(this.getCenter())) < this.size / 2 - distance - 1;

	return position.x >= distance && position.y >= distance && position.x < this.size - distance && position.y < this.size - distance;
};

/**
 * Determines whether the given coordinates are within the given distance of the passable map area.
 * Should be used to restrict entity placement and path creation.
 */
RandomMap.prototype.validTilePassable = function(position, distance = 0)
{
	return this.validTile(position, distance + MAP_BORDER_WIDTH);
};

/**
 * Determines whether the given coordinates are within the tile grid, passable or not.
 * Should be used to restrict texture painting.
 */
RandomMap.prototype.inMapBounds = function(position)
{
	return position.x >= 0 && position.y >= 0 && position.x < this.size && position.y < this.size;
};

/**
 * Determines whether the given coordinates are within the heightmap grid.
 * Should be used to restrict elevation changes.
 */
RandomMap.prototype.validHeight = function(position)
{
	if (position.x < 0 || position.y < 0)
		return false;

	if (TILE_CENTERED_HEIGHT_MAP)
		return position.x < this.size && position.y < this.size;

	return position.x <= this.size && position.y <= this.size;
};

/**
 * Returns a random point on the map.
 * @param passableOnly - Should be true for entity placement and false for terrain or elevation operations.
 */
RandomMap.prototype.randomCoordinate = function(passableOnly)
{
	let border = passableOnly ? MAP_BORDER_WIDTH : 0;

	if (this.isCircularMap())
		// Polar coordinates
		// Uniformly distributed on the disk
		return Vector2D.add(
			this.getCenter(),
			new Vector2D((this.size / 2 - border) * Math.sqrt(randFloat(0, 1)), 0).rotate(randomAngle()).floor());

	// Rectangular coordinates
	return new Vector2D(
		randIntExclusive(border, this.size - border),
		randIntExclusive(border, this.size - border));
};

/**
 * Returns the name of the texture of the given tile.
 */
RandomMap.prototype.getTexture = function(position)
{
	if (!this.validTile(position))
		throw new Error("getTexture: invalid tile position " + uneval(position));

	return this.IDToName[this.texture[position.x][position.y]];
};

/**
 * Paints the given texture on the given tile.
 */
RandomMap.prototype.setTexture = function(position, texture)
{
	if (position.x < 0 ||
	    position.y < 0 ||
	    position.x >= this.texture.length ||
	    position.y >= this.texture[position.x].length)
		throw new Error("setTexture: invalid tile position " + uneval(position));

	this.texture[position.x][position.y] = this.getTextureID(texture);
};

RandomMap.prototype.getHeight = function(position)
{
	if (!this.validHeight(position))
		throw new Error("getHeight: invalid vertex position " + uneval(position));

	return this.height[position.x][position.y];
};

RandomMap.prototype.setHeight = function(position, height)
{
	if (!this.validHeight(position))
		throw new Error("setHeight: invalid vertex position " + uneval(position));

	this.height[position.x][position.y] = height;
};

/**
 * Adds the given Entity to the map at the location it defines, even if at the impassable map border.
 */
RandomMap.prototype.placeEntityAnywhere = function(templateName, playerID, position, orientation)
{
	let entity = new Entity(this.getEntityID(), templateName, playerID, position, orientation);
	this.entities.push(entity);
};

/**
 * Adds the given Entity to the map at the location it defines, if that area is not at the impassable map border.
 */
RandomMap.prototype.placeEntityPassable = function(templateName, playerID, position, orientation)
{
	if (g_Map.validTilePassable(position))
		this.placeEntityAnywhere(templateName, playerID, position, orientation);
};

/**
 * Returns the Entity that was painted by a Terrain class on the given tile or undefined otherwise.
 */
RandomMap.prototype.getTerrainEntity = function(position)
{
	if (!this.validTilePassable(position))
		throw new Error("getTerrainEntity: invalid tile position " + uneval(position));

	return this.terrainEntities[position.x][position.y];
};

/**
 * Places the Entity on the given tile and allows to later replace it if the terrain was painted over.
 */
RandomMap.prototype.setTerrainEntity = function(templateName, playerID, position, orientation)
{
	let tilePosition = position.clone().floor();
	if (!this.validTilePassable(tilePosition))
		throw new Error("setTerrainEntity: invalid tile position " + uneval(position));

	this.terrainEntities[tilePosition.x][tilePosition.y] =
		new Entity(this.getEntityID(), templateName, playerID, position, orientation);
};

/**
 * Constructs a new Area object and informs the Map which points correspond to this area.
 */
RandomMap.prototype.createArea = function(points)
{
	let areaID = ++this.areaID;
	for (let p of points)
		this.area[p.x][p.y] = areaID;
	return new Area(points, areaID);
};

RandomMap.prototype.createTileClass = function()
{
	return new TileClass(this.size);
};

/**
 * Retrieve interpolated height for arbitrary coordinates within the heightmap grid.
 */
RandomMap.prototype.getExactHeight = function(position)
{
	let xi = Math.min(Math.floor(position.x), this.size);
	let zi = Math.min(Math.floor(position.y), this.size);
	let xf = position.x - xi;
	let zf = position.y - zi;

	let h00 = this.height[xi][zi];
	let h01 = this.height[xi][zi + 1];
	let h10 = this.height[xi + 1][zi];
	let h11 = this.height[xi + 1][zi + 1];

	return (1 - zf) * ((1 - xf) * h00 + xf * h10) + zf * ((1 - xf) * h01 + xf * h11);
};

// Converts from the tile centered height map to the corner based height map, used when TILE_CENTERED_HEIGHT_MAP = true
RandomMap.prototype.cornerHeight = function(position)
{
	let count = 0;
	let sumHeight = 0;

	for (let dir of [[-1, -1], [-1, 0], [0, -1], [0, 0]])
	{
		let pos = Vector2D.add(position, new Vector2D(dir[0], dir[1]));
		if (this.validHeight(pos))
		{
			++count;
			sumHeight += this.getHeight(pos);
		}
	}

	if (!count)
		return 0;

	return sumHeight / count;
};

RandomMap.prototype.getAdjacentPoints = function(position)
{
	let adjacentPositions = [];

	for (let x = -1; x <= 1; ++x)
		for (let z = -1; z <= 1; ++z)
			if (x || z )
			{
				let adjacentPos = Vector2D.add(position, new Vector2D(x, z)).round();
				if (this.inMapBounds(adjacentPos))
					adjacentPositions.push(adjacentPos);
			}

	return adjacentPositions;
};

/**
 * Returns the average height of adjacent tiles, helpful for smoothing.
 */
RandomMap.prototype.getAverageHeight = function(position)
{
	let adjacentPositions = this.getAdjacentPoints(position);
	if (!adjacentPositions.length)
		return 0;

	return adjacentPositions.reduce((totalHeight, pos) => totalHeight + this.getHeight(pos), 0) / adjacentPositions.length;
};

/**
 * Returns the steepness of the given location, defined as the average height difference of the adjacent tiles.
 */
RandomMap.prototype.getSlope = function(position)
{
	let adjacentPositions = this.getAdjacentPoints(position);
	if (!adjacentPositions.length)
		return 0;

	return adjacentPositions.reduce((totalSlope, adjacentPos) =>
		totalSlope + Math.abs(this.getHeight(adjacentPos) - this.getHeight(position)), 0) / adjacentPositions.length;
};

/**
 * Retrieve an array of all Entities placed on the map.
 */
RandomMap.prototype.exportEntityList = function()
{
	// Change rotation from simple 2d to 3d befor giving to engine
	for (let entity of this.entities)
		entity.rotation.y = Math.PI / 2 - entity.rotation.y;

	// Terrain objects e.g. trees
	for (let x = 0; x < this.size; ++x)
		for (let z = 0; z < this.size; ++z)
			if (this.terrainEntities[x][z])
				this.entities.push(this.terrainEntities[x][z]);

	this.logger.printDirectly("Total entities: " + this.entities.length + ".\n")
	return this.entities;
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
			let position = new Vector2D(x, z);
			let currentHeight = TILE_CENTERED_HEIGHT_MAP ? this.cornerHeight(position) : this.getHeight(position);

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

RandomMap.prototype.ExportMap = function()
{
	if (g_Environment.Water.WaterBody.Height === undefined)
		g_Environment.Water.WaterBody.Height = SEA_LEVEL - 0.1;

	this.logger.close();

	Engine.ExportMap({
		"entities": this.exportEntityList(),
		"height": this.exportHeightData(),
		"seaLevel": SEA_LEVEL,
		"size": this.size,
		"textureNames": this.IDToName,
		"tileData": this.exportTerrainTextures(),
		"Camera": g_Camera,
		"Environment": g_Environment
	});
};
