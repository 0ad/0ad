/**
 * @file A Constraint decides if a tile satisfies a condition defined by the class.
 */

/**
 * The NullConstraint is always satisfied.
 */
function NullConstraint() {}

NullConstraint.prototype.allows = function(x, z)
{
	return true;
};

/**
 * The AndConstraint is met if every given Constraint is satisfied by the tile.
 */
function AndConstraint(constraints)
{
	if (constraints instanceof Array)
		this.constraints = constraints
	else if (!constraints)
		this.constraints = [];
	else
		this.constraints = [constraints];
}

AndConstraint.prototype.allows = function(x, z)
{
	return this.constraints.every(constraint => constraint.allows(x, z));
};

/**
 * The AvoidAreaConstraint is met if the tile is not part of the given Area.
 */
function AvoidAreaConstraint(area)
{
	this.area = area;
}

AvoidAreaConstraint.prototype.allows = function(x, z)
{
	return g_Map.area[x][z] != this.area.getID();
};

/**
 * The AvoidTextureConstraint is met if the terrain texture of the tile is different from the given texture.
 */
function AvoidTextureConstraint(textureID)
{
	this.textureID = textureID;
}

AvoidTextureConstraint.prototype.allows = function(x, z)
{
	return g_Map.texture[x][z] != this.textureID;
};

/**
 * The AvoidTileClassConstraint is met if there are no tiles marked with the given TileClass within the given radius of the tile.
 */
function AvoidTileClassConstraint(tileClassID, distance)
{
	this.tileClass = getTileClass(tileClassID);
	this.distance = distance;
}

AvoidTileClassConstraint.prototype.allows = function(x, z)
{
	return this.tileClass.countMembersInRadius(x, z, this.distance) == 0;
};

/**
 * The StayInTileClassConstraint is met if every tile within the given radius of the tile is marked with the given TileClass.
 */
function StayInTileClassConstraint(tileClassID, distance)
{
	this.tileClass = getTileClass(tileClassID);
	this.distance = distance;
}

StayInTileClassConstraint.prototype.allows = function(x, z)
{
	return this.tileClass.countNonMembersInRadius(x, z, this.distance) == 0;
};

/**
 * The BorderTileClassConstraint is met if there are
 * tiles not marked with the given TileClass within distanceInside of the tile and
 * tiles marked with the given TileClass within distanceOutside of the tile.
 */
function BorderTileClassConstraint(tileClassID, distanceInside, distanceOutside)
{
	this.tileClass = getTileClass(tileClassID);
	this.distanceInside = distanceInside;
	this.distanceOutside = distanceOutside;
}

BorderTileClassConstraint.prototype.allows = function(x, z)
{
	return this.tileClass.countMembersInRadius(x, z, this.distanceOutside) > 0 &&
	       this.tileClass.countNonMembersInRadius(x, z, this.distanceInside) > 0;
};

/**
 * The HeightConstraint is met if the elevation of the tile is within the given range.
 * One can pass Infinity to only test for one side.
 */
function HeightConstraint(minHeight, maxHeight)
{
	this.minHeight = minHeight;
	this.maxHeight = maxHeight;
}

HeightConstraint.prototype.allows = function(x, z)
{
	return this.minHeight <= g_Map.height[x][z] && g_Map.height[x][z] <= this.maxHeight;
};
