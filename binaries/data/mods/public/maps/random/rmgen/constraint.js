
///////////////////////////////////////////////////////////////////////////
//	NullConstraints: No constraint - always return true
///////////////////////////////////////////////////////////////////////////
function NullConstraint() {}

NullConstraint.prototype.allows = function(x, y)
{
	return true;
};

///////////////////////////////////////////////////////////////////////////
//	AndConstraints: Check multiple constraints	
///////////////////////////////////////////////////////////////////////////
function AndConstraint(constraints)
{
	this.constraints = constraints;
}

AndConstraint.prototype.allows = function(x, y)
{
	for (var i=0; i < this.constraints.length; ++i)
	{
		if (!this.constraints[i].allows(x, y))
			return false;
	}
	
	return true;
};

///////////////////////////////////////////////////////////////////////////
//	AvoidAreaConstraint
///////////////////////////////////////////////////////////////////////////
function AvoidAreaConstraint(area)
{
	this.area = area;
}

AvoidAreaConstraint.prototype.allows = function(x, y)
{
	return g_Map.area[x][y] != this.area;
};

///////////////////////////////////////////////////////////////////////////
//	AvoidTextureConstraint
///////////////////////////////////////////////////////////////////////////
function AvoidTextureConstraint(textureID)
{
	this.textureID = textureID;
}

AvoidTextureConstraint.prototype.allows = function(x, y)
{
	return g_Map.texture[x][y] != this.textureID;
};

///////////////////////////////////////////////////////////////////////////
//	AvoidTileClassConstraint
///////////////////////////////////////////////////////////////////////////
function AvoidTileClassConstraint(tileClassID, distance)
{
	this.tileClass = getTileClass(tileClassID);
	this.distance = distance;
}

AvoidTileClassConstraint.prototype.allows = function(x, y)
{
	return this.tileClass.countMembersInRadius(x, y, this.distance) == 0;
};

///////////////////////////////////////////////////////////////////////////
//	StayInTileClassConstraint
///////////////////////////////////////////////////////////////////////////
function StayInTileClassConstraint(tileClassID, distance)
{
	this.tileClass = getTileClass(tileClassID);
	this.distance = distance;
}

StayInTileClassConstraint.prototype.allows = function(x, y)
{
	return this.tileClass.countNonMembersInRadius(x, y, this.distance) == 0;
};

///////////////////////////////////////////////////////////////////////////
//	BorderTileClassConstraint
///////////////////////////////////////////////////////////////////////////
function BorderTileClassConstraint(tileClassID, distanceInside, distanceOutside)
{
	this.tileClass = getTileClass(tileClassID);
	this.distanceInside = distanceInside;
	this.distanceOutside = distanceOutside;
}

BorderTileClassConstraint.prototype.allows = function(x, y)
{
	return (this.tileClass.countMembersInRadius(x, y, this.distanceOutside) > 0 
		&& this.tileClass.countNonMembersInRadius(x, y, this.distanceInside) > 0);
};
