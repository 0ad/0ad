//////////////////////////////////////////////////////////////////////
//	RangeOp
//
//	Class for efficiently finding number of points within a range
//
//////////////////////////////////////////////////////////////////////

function RangeOp(size)
{
	// Get smallest power of 2 which is greater than or equal to size
	this.nn = 1;
	while (this.nn < size) {
		this.nn *= 2;
	}
	
	this.vals = new Int16Array(2*this.nn);	// int16
}

RangeOp.prototype.set = function(pos, amt)
{
	this.add(pos, amt - this.vals[this.nn + pos]);
};

RangeOp.prototype.add = function(pos, amt)
{
	for(var s = this.nn; s >= 1; s /= 2)
	{
		this.vals[s + pos] += amt;
		pos = Math.floor(pos/2);
	}
};

RangeOp.prototype.get = function(start, end)
{
	var ret = 0;
	var i = 1;
	var nn = this.nn;
	
	// Count from start to end by powers of 2
	for (; start+i <= end; i *= 2)
	{
		if (start & i)
		{	// For each bit in start
			ret += this.vals[nn/i + Math.floor(start/i)];
			start += i;
		}
	}
	
	// 
	while(i >= 1)
	{
		if(start+i <= end)
		{
			ret += this.vals[nn/i + Math.floor(start/i)];
			start += i;
		}
		i /= 2;
	}
	
	return ret;
};


//////////////////////////////////////////////////////////////////////
//	TileClass
//
//	Class for representing terrain types and containing all the tiles
//		within that type
//
//////////////////////////////////////////////////////////////////////

function TileClass(size, id)
{
	this.id = id;
	this.size = size;
	this.inclusionCount = new Array(size);
	this.rangeCount = new Array(size);
	
	for (var i=0; i < size; ++i)
	{
		this.inclusionCount[i] = new Int16Array(size); //int16
		this.rangeCount[i] = new RangeOp(size);
	}
}

TileClass.prototype.add = function(x, z)
{
	if (!this.inclusionCount[x][z] && g_Map.validT(x, z))
	{
		this.rangeCount[z].add(x, 1);
	}
	
	this.inclusionCount[x][z]++;
};

TileClass.prototype.remove = function(x, z)
{
	this.inclusionCount[x][z]--;
	if(!this.inclusionCount[x][z])
	{
		this.rangeCount[z].add(x, -1);
	}
};

TileClass.prototype.countInRadius = function(cx, cy, radius, returnMembers)
{
	var members = 0;
	var nonMembers = 0;
	var size = this.size;

	var ymax = cy+radius;
	var radius2 = radius*radius;
	
	for (var y = cy-radius; y <= ymax; y++)
	{
		var iy = Math.floor(y);
		if (radius >= 27) // Switchover point before RangeOp actually performs better than a straight algorithm
		{
			if(iy >= 0 && iy < size)
			{
				var dy = y - cy;
				var dx = Math.sqrt(radius*radius - dy*dy);
				
				var lowerX = Math.floor(cx - dx);
				var upperX = Math.floor(cx + dx);
				
				var minX = (lowerX > 0 ? lowerX : 0);		
				var maxX = (upperX < size ? upperX+1 : size);
				
				var total = maxX - minX;
				var mem = this.rangeCount[iy].get(minX, maxX);
				
				members += mem;
				nonMembers += total - mem;
			}
		}
		else // Simply check the tiles one by one to find the number
		{
			var xmax = cx + radius;
			for (var x = cx-radius; x <= xmax; x++)
			{
				var ix = Math.floor(x);
				var dx = (ix - cx);
				var dy = (iy - cy);
				if (dx*dx + dy*dy <= radius2)
					if (this.inclusionCount[ix] && this.inclusionCount[ix][iy] && this.inclusionCount[ix][iy] > 0)
					{
						members += 1;
					}
					else
						nonMembers += 1;
			}
		}
	}
	
	if (returnMembers)
		return members;
	else
		return nonMembers;
};

TileClass.prototype.countMembersInRadius = function(cx, cy, radius)
{
	return this.countInRadius(cx, cy, radius, true);
};

TileClass.prototype.countNonMembersInRadius = function(cx, cy, radius)
{
	return this.countInRadius(cx, cy, radius, false);
};
