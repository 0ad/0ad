/**
 * @file A Constraint decides if a tile satisfies a condition defined by the class.
 */

/**
 * The NullConstraint is always satisfied.
 */
function NullConstraint() {}

NullConstraint.prototype.allows = function(position)
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

AndConstraint.prototype.allows = function(position)
{
	return this.constraints.every(constraint => constraint.allows(position));
};

/**
 * The AvoidAreaConstraint is met if the tile is not part of the given Area.
 */
function AvoidAreaConstraint(area)
{
	this.area = area;
}

AvoidAreaConstraint.prototype.allows = function(position)
{
	return g_Map.area[position.x][position.y] != this.area.getID();
};

/**
 * The StayTextureConstraint is met if the tile has the given texture.
 */
function StayTextureConstraint(texture)
{
	this.texture = texture;
}

StayTextureConstraint.prototype.allows = function(position)
{
	return g_Map.getTexture(position) == this.texture;
};

/**
 * The AvoidTextureConstraint is met if the terrain texture of the tile is different from the given texture.
 */
function AvoidTextureConstraint(texture)
{
	this.texture = texture;
}

AvoidTextureConstraint.prototype.allows = function(position)
{
	return g_Map.getTexture(position) != this.texture;
};

/**
 * The AvoidTileClassConstraint is met if there are no tiles marked with the given TileClass within the given radius of the tile.
 */
function AvoidTileClassConstraint(tileClass, distance)
{
	this.tileClass = tileClass;
	this.distance = distance;
}

AvoidTileClassConstraint.prototype.allows = function(position)
{
	return this.tileClass.countMembersInRadius(position, this.distance) == 0;
};

/**
 * The StayInTileClassConstraint is met if every tile within the given radius of the tile is marked with the given TileClass.
 */
function StayInTileClassConstraint(tileClass, distance)
{
	this.tileClass = tileClass;
	this.distance = distance;
}

StayInTileClassConstraint.prototype.allows = function(position)
{
	return this.tileClass.countNonMembersInRadius(position, this.distance) == 0;
};

/**
 * The NearTileClassConstraint is met if at least one tile within the given radius of the tile is marked with the given TileClass.
 */
function NearTileClassConstraint(tileClass, distance)
{
	this.tileClass = tileClass;
	this.distance = distance;
}

NearTileClassConstraint.prototype.allows = function(position)
{
	return this.tileClass.countMembersInRadius(position, this.distance) > 0;
};

/**
 * The BorderTileClassConstraint is met if there are
 * tiles not marked with the given TileClass within distanceInside of the tile and
 * tiles marked with the given TileClass within distanceOutside of the tile.
 */
function BorderTileClassConstraint(tileClass, distanceInside, distanceOutside)
{
	this.tileClass = tileClass;
	this.distanceInside = distanceInside;
	this.distanceOutside = distanceOutside;
}

BorderTileClassConstraint.prototype.allows = function(position)
{
	return this.tileClass.countMembersInRadius(position, this.distanceOutside) > 0 &&
	       this.tileClass.countNonMembersInRadius(position, this.distanceInside) > 0;
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

HeightConstraint.prototype.allows = function(position)
{
	return this.minHeight <= g_Map.getHeight(position) && g_Map.getHeight(position) <= this.maxHeight;
};

/**
 * The SlopeConstraint is met if the steepness of the terrain is within the given range.
 */
function SlopeConstraint(minSlope, maxSlope)
{
	this.minSlope = minSlope;
	this.maxSlope = maxSlope;
}

SlopeConstraint.prototype.allows = function(position)
{
	return this.minSlope <= g_Map.getSlope(position) && g_Map.getSlope(position) <= this.maxSlope;
};
