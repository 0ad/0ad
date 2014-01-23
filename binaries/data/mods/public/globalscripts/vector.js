/////////////////////////////////////////////////////////////////////
//	Vector2D
//
//	Class for representing and manipulating 2D vectors
//
/////////////////////////////////////////////////////////////////////

// TODO: Type errors if v not instanceof Vector classes
// TODO: Possibly implement in C++

function Vector2D(x, y)
{
	if (arguments.length == 2)
		this.set(x, y);
	else
		this.set(0, 0);
}

Vector2D.prototype.clone = function()
{
	return new Vector2D(this.x, this.y);
};

Vector2D.prototype.set = function(x, y)
{
	this.x = x;
	this.y = y;
	return this;
};

Vector2D.prototype.add = function(v)
{
	this.x += v.x;
	this.y += v.y;
	return this;
};

Vector2D.prototype.sub = function(v)
{
	this.x -= v.x;
	this.y -= v.y;
	return this;
};

Vector2D.prototype.mult = function(f)
{
	this.x *= f;
	this.y *= f;
	return this;
};

Vector2D.prototype.div = function(f)
{
	this.x /= f;
	this.y /= f;
	return this;
};

Vector2D.prototype.dot = function(v)
{
	return this.x * v.x + this.y * v.y;
};

// get the non-zero coordinate of the vector cross
Vector2D.prototype.cross = function(v)
{
	return this.x * v.y - this.y * v.x;
};

Vector2D.prototype.lengthSquared = function()
{
	return this.dot(this);
};

Vector2D.prototype.length = function()
{
	return Math.sqrt(this.lengthSquared());
};

/**
 * Compare this length to the length of v,
 * @return 0 if the lengths are equal
 * @return 1 if this is longer than v
 * @return -1 if this is shorter than v
 * @return NaN if the vectors aren't comparable
 */
Vector2D.prototype.compareLength = function(v)
{
	var dDist = this.lengthSquared() - v.lengthSquared();
	if (!dDist)
		return dDist == 0 ? 0 : NaN;
	return dDist < 0 ? -1 : 1;
};

Vector2D.prototype.distanceToSquared = function(v)
{
	var dx = this.x - v.x;
	var dy = this.y - v.y;
	return dx * dx + dy * dy;
};

Vector2D.prototype.distanceTo = function(v)
{
	return Math.sqrt(this.distanceToSquared(v));
};

Vector2D.prototype.normalize = function()
{
	var mag = this.length();
	if (!mag)
		return this;

	return this.div(mag);
};

/////////////////////////////////////////////////////////////////////
//	Vector3D
//
//	Class for representing and manipulating 3D vectors
//
/////////////////////////////////////////////////////////////////////

function Vector3D(x, y, z)
{
	if (arguments.length == 3)
		this.set(x, y, y);
	else
		this.set(0, 0, 0);
}

Vector3D.prototype.clone = function()
{
	return new Vector3D(this.x, this.y, this.z);
};

Vector3D.prototype.set = function(x, y, z)
{
	this.x = x;
	this.y = y;
	this.z = z;
	return this;
};

Vector3D.prototype.add = function(v)
{
	this.x += v.x;
	this.y += v.y;
	this.z += v.z;
	return this;
};

Vector3D.prototype.sub = function(v)
{
	this.x -= v.x;
	this.y -= v.y;
	this.z -= v.z;
	return this;
};

Vector3D.prototype.mult = function(f)
{
	this.x *= f;
	this.y *= f;
	this.z *= f;
	return this;
};

Vector3D.prototype.div = function(f)
{
	this.x /= f;
	this.y /= f;
	this.z /= f;
	return this;
};

Vector3D.prototype.dot = function(v)
{
	return this.x * v.x + this.y * v.y + this.z * v.z;
};

Vector3D.prototype.lengthSquared = function()
{
	return this.dot(this);
};

Vector3D.prototype.length = function()
{
	return Math.sqrt(this.lengthSquared());
};

/**
 * Compare this length to the length of v,
 * @return 0 if the lengths are equal
 * @return 1 if this is longer than v
 * @return -1 if this is shorter than v
 * @return NaN if the vectors aren't comparable
 */
Vector3D.prototype.compareLength = function(v)
{
	var dDist = this.lengthSquared() - v.lengthSquared();
	if (!dDist)
		return dDist == 0 ? 0 : NaN;
	return dDist < 0 ? -1 : 1;
};

Vector3D.prototype.distanceToSquared = function(v)
{
	var dx = this.x - v.x;
	var dy = this.y - v.y;
	var dz = this.z - v.z;
	return dx * dx + dy * dy + dz * dz;
};

Vector3D.prototype.distanceTo = function(v)
{
	return Math.sqrt(this.distanceToSquared(v));
};

Vector3D.prototype.normalize = function()
{
	var mag = this.length();
	if (!mag)
		return this;
	
	return this.div(mag);
};

// make the prototypes easily accessible to C++
const Vector2Dprototype = Vector2D.prototype;
const Vector3Dprototype = Vector3D.prototype;
