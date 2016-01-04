function GlobalSubtractionHelper(a, b)
{
	return a-b;
}

function Vector2D(x, y)
{
	if (arguments.length == 2)
	{
		this.x = x;
		this.y = y;
	}
	else
		this.x = this.y = 0;
}

Vector2D.prototype.add = function(v)
{
	this.x += v.x;
	this.y += v.y;
	return this;
};

function Vector3D(x, y, z)
{
	if (arguments.length == 3)
	{
		this.x = x;
		this.y = y;
		this.z = z;
	}
	else
		this.x = this.y = this.z = 0;
}
Vector3D.prototype.add = function(v)
{
	this.x += v.x;
	this.y += v.y;
	this.z += v.z;
	return this;
};

// make the prototypes easily accessible to C++
const Vector2Dprototype = Vector2D.prototype;
const Vector3Dprototype = Vector3D.prototype;
