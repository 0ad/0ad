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

/**
 * Class that can be tagged to any tile. Can be used to constrain placers and entity placement to given areas.
 */
function TileClass(size)
{
	this.size = size;
	this.inclusionCount = [];
	this.rangeCount = [];

	for (let i=0; i < size; ++i)
	{
		this.inclusionCount[i] = new Int16Array(size); //int16
		this.rangeCount[i] = new RangeOp(size);
	}
}

TileClass.prototype.has = function(position)
{
	return !!this.inclusionCount[position.x] && !!this.inclusionCount[position.x][position.y];
};

TileClass.prototype.add = function(position)
{
	if (!this.inclusionCount[position.x][position.y] && g_Map.validTile(position))
		this.rangeCount[position.y].add(position.x, 1);

	++this.inclusionCount[position.x][position.y];
};

TileClass.prototype.remove = function(position)
{
	--this.inclusionCount[position.x][position.y];

	if (!this.inclusionCount[position.x][position.y])
		this.rangeCount[position.y].add(position.x, -1);
};

TileClass.prototype.countInRadius = function(position, radius, returnMembers)
{
	let members = 0;
	let nonMembers = 0;
	let radius2 = Math.square(radius);

	for (let y = position.y - radius; y <= position.y + radius; ++y)
	{
		let iy = Math.floor(y);
		if (radius >= 27) // Switchover point before RangeOp actually performs better than a straight algorithm
		{
			if (iy >= 0 && iy < this.size)
			{
				let dx = Math.sqrt(Math.square(radius) - Math.square(y - position.y));

				let minX = Math.max(Math.floor(position.x - dx), 0);
				let maxX = Math.min(Math.floor(position.x + dx), this.size - 1) + 1;

				let newMembers = this.rangeCount[iy].get(minX, maxX);

				members += newMembers;
				nonMembers += maxX - minX - newMembers;
			}
		}
		else // Simply check the tiles one by one to find the number
		{
			let dy = iy - position.y;

			let xMin = Math.max(Math.floor(position.x - radius), 0);
			let xMax = Math.min(Math.ceil(position.x + radius), this.size - 1);

			for (let ix = xMin; ix <= xMax; ++ix)
			{
				let dx = ix - position.x;
				if (Math.square(dx) + Math.square(dy) <= radius2)
				{
					if (this.inclusionCount[ix] && this.inclusionCount[ix][iy] && this.inclusionCount[ix][iy] > 0)
						++members;
					else
						++nonMembers;
				}
			}
		}
	}

	if (returnMembers)
		return members;
	else
		return nonMembers;
};

TileClass.prototype.countMembersInRadius = function(position, radius)
{
	return this.countInRadius(position, radius, true);
};

TileClass.prototype.countNonMembersInRadius = function(position, radius)
{
	return this.countInRadius(position, radius, false);
};
