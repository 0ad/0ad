/////////////////////////////////////////////////////////////////////
//	Vector2D
/////////////////////////////////////////////////////////////////////

// TODO: Type errors if v not instanceof Vector classes
// TODO: Possible implement in C++

function Vector2D(x, y)
{
	if (arguments.length == 2)
	{
		this.set(x, y);
	}
	else
	{
		this.set(0, 0);
	}
}

Vector2D.prototype.set = function(x, y)
{
	this.x = x;
	this.y = y;
};

Vector2D.prototype.add = function(v)
{
	return new Vector2D(this.x + v.x, this.y + v.y);
};

Vector2D.prototype.sub = function(v)
{
	return new Vector2D(this.x - v.x, this.y - v.y);
};

Vector2D.prototype.mult = function(f)
{
	return new Vector2D(this.x * f, this.y * f);
};

Vector2D.prototype.div = function(f)
{
	return new Vector2D(this.x / f, this.y / f);
};

Vector2D.prototype.dot = function(v)
{
	return this.x * v.x + this.y * v.y;
};

Vector2D.prototype.lengthSquared = function()
{
	return this.dot(this);
};

Vector2D.prototype.length = function()
{
	return sqrt(this.lengthSquared());
};

Vector2D.prototype.normalize = function()
{
	var mag = this.length();
	
	this.x /= mag;
	this.y /= mag;
};

/////////////////////////////////////////////////////////////////////
//	Vector3D
/////////////////////////////////////////////////////////////////////

function Vector3D(x, y, z)
{
	if (arguments.length == 3)
	{
		this.set(x, y, z);
	}
	else
	{
		this.set(0, 0, 0);
	}
}

Vector3D.prototype.set = function(x, y, z)
{
	this.x = x;
	this.y = y;
	this.z = z;
};

Vector3D.prototype.add = function(v)
{
	return new Vector3D(this.x + v.x, this.y + v.y, this.z + v.z);
};

Vector3D.prototype.sub = function(v)
{
	return new Vector3D(this.x - v.x, this.y - v.y, this.z - v.z);
};

Vector3D.prototype.mult = function(f)
{
	return new Vector3D(this.x * f, this.y * f, this.z * f);
};

Vector3D.prototype.div = function(f)
{
	return new Vector3D(this.x / f, this.y / f, this.z / f);
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
	return sqrt(this.lengthSquared());
};

Vector3D.prototype.normalize = function()
{
	var mag = this.length();
	
	this.x /= mag;
	this.y /= mag;
	this.z /= mag;
};

