var API3 = function(m)
{

/**
 * The map module.
 * Copied with changes from QuantumState's original for qBot, it's a component for storing 8 bit values.
 */

/** The function needs to be named too because of the copyConstructor functionality */
m.Map = function Map(sharedScript, type, originalMap, actualCopy)
{
	// get the correct dimensions according to the map type
	let map = (type === "territory" || type === "resource") ? sharedScript.territoryMap : sharedScript.passabilityMap;
	this.width = map.width;
	this.height = map.height;
	this.cellSize = map.cellSize;
	this.length = this.width * this.height;

	this.maxVal = 255;

	// sanity check
	if (originalMap && originalMap.length !== this.length)
		warn("AI map size incompatibility with type " + type + ": original " + originalMap.length + " new " + this.length);

	if (originalMap && actualCopy)
	{
		this.map = new Uint8Array(this.length);
		for (let i = 0; i < this.length; ++i)
			this.map[i] = originalMap[i];
	}
	else if (originalMap)
		this.map = originalMap;
	else
		this.map = new Uint8Array(this.length);
};

m.Map.prototype.setMaxVal = function(val)
{
	this.maxVal = val;
};

m.Map.prototype.gamePosToMapPos = function(p)
{
	return [Math.floor(p[0]/this.cellSize), Math.floor(p[1]/this.cellSize)];
};

m.Map.prototype.point = function(p)
{
	let q = this.gamePosToMapPos(p);
	q[0] = q[0] >= this.width ? this.width-1 : (q[0] < 0 ? 0 : q[0]);
	q[1] = q[1] >= this.width ? this.width-1 : (q[1] < 0 ? 0 : q[1]);
	return this.map[q[0] + this.width * q[1]];
};

m.Map.prototype.addInfluence = function(cx, cy, maxDist, strength, type = "linear")
{
	strength = strength ? strength : maxDist;

	let x0 = Math.floor(Math.max(0, cx - maxDist));
	let y0 = Math.floor(Math.max(0, cy - maxDist));
	let x1 = Math.floor(Math.min(this.width-1, cx + maxDist));
	let y1 = Math.floor(Math.min(this.height-1, cy + maxDist));
	let maxDist2 = maxDist * maxDist;

	// code duplicating for speed
	if (type === "linear")
	{
		let str = strength / maxDist;	
		for (let y = y0; y < y1; ++y)
		{
			let dy = y - cy;		
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let quant = str * (maxDist - Math.sqrt(r2));
				let w = x + y * this.width;
				if (this.map[w] + quant < 0)
					this.map[w] = 0;
				else if (this.map[w] + quant > this.maxVal)
					this.map[w] = this.maxVal;	// avoids overflow.
				else
					this.map[w] += quant;
			}
		}
	}
	else if (type === "quadratic")
	{
		let str = strength / maxDist2;
		for (let y = y0; y < y1; ++y)
		{
			let dy = y - cy;		
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let quant = str * (maxDist2 - r2);
				let w = x + y * this.width;				    
				if (this.map[w] + quant < 0)
					this.map[w] = 0;
				else if (this.map[w] + quant > this.maxVal)
					this.map[w] = this.maxVal;	// avoids overflow.
				else
					this.map[w] += quant;
			}
		}
	}
	else
	{
		for (let y = y0; y < y1; ++y)
		{
			let dy = y - cy;
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let w = x + y * this.width;				
				if (this.map[w] + strength < 0)
					this.map[w] = 0;
				else if (this.map[w] + strength > this.maxVal)
					this.map[w] = this.maxVal;	// avoids overflow.
				else
					this.map[w] += strength;
			}
		}
	}
};

m.Map.prototype.multiplyInfluence = function(cx, cy, maxDist, strength, type = "constant")
{
	strength = strength ? +strength : +maxDist;

	let x0 = Math.max(0, cx - maxDist);
	let y0 = Math.max(0, cy - maxDist);
	let x1 = Math.min(this.width, cx + maxDist);
	let y1 = Math.min(this.height, cy + maxDist);
	let maxDist2 = maxDist * maxDist;

	if (type === "linear")
	{
		let str = strength / maxDist;
		for (let y = y0; y < y1; ++y)
		{
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let dy = y - cy;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let quant = str * (maxDist - Math.sqrt(r2)) * this.map[x + y * this.width];
				if (quant < 0)
					this.map[x + y * this.width] = 0;
				else if (quant > this.maxVal)
					this.map[x + y * this.width] = this.maxVal;
				else
					this.map[x + y * this.width] = quant;
			}
		}
	}
	else if (type === "quadratic")
	{
		let str = strength / maxDist2;
		for (let y = y0; y < y1; ++y)
		{
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let dy = y - cy;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let quant = str * (maxDist2 - r2) * this.map[x + y * this.width];
				if (quant < 0)
					this.map[x + y * this.width] = 0;
				else if (quant > this.maxVal)
					this.map[x + y * this.width] = this.maxVal;
				else
					this.map[x + y * this.width] = quant;
			}
		}
	}
	else
	{
		for (let y = y0; y < y1; ++y)
		{
			for (let x = x0; x < x1; ++x)
			{
				let dx = x - cx;
				let dy = y - cy;
				let r2 = dx*dx + dy*dy;
				if (r2 >= maxDist2)
					continue;
				let quant = this.map[x + y * this.width] * strength;
				if (quant < 0)
					this.map[x + y * this.width] = 0;
				else if (quant > this.maxVal)
					this.map[x + y * this.width] = this.maxVal;
				else
					this.map[x + y * this.width] = quant;
			}
		}
	}
};

/** doesn't check for overflow. */
m.Map.prototype.setInfluence = function(cx, cy, maxDist, value = 0)
{
	let x0 = Math.max(0, cx - maxDist);
	let y0 = Math.max(0, cy - maxDist);
	let x1 = Math.min(this.width, cx + maxDist);
	let y1 = Math.min(this.height, cy + maxDist);
	let maxDist2 = maxDist * maxDist;

	for (let y = y0; y < y1; ++y)
	{
		for (let x = x0; x < x1; ++x)
		{
			let dx = x - cx;
			let dy = y - cy;
			if (dx*dx + dy*dy < maxDist2)
				this.map[x + y * this.width] = value;
		}
	}
};

m.Map.prototype.sumInfluence = function(cx, cy, radius)
{
	let x0 = Math.max(0, cx - radius);
	let y0 = Math.max(0, cy - radius);
	let x1 = Math.min(this.width, cx + radius);
	let y1 = Math.min(this.height, cy + radius);
	let radius2 = radius * radius;

	let sum = 0;
	for (let y = y0; y < y1; ++y)
	{
		for (let x = x0; x < x1; ++x)
		{
			let dx = x - cx;
			let dy = y - cy;
			if (dx*dx + dy*dy < radius2)
				sum += this.map[x + y * this.width];
		}
	}
	return sum;
};
/**
 * Make each cell's 16-bit/8-bit value at least one greater than each of its
 * neighbours' values. (If the grid is initialised with 0s and 65535s or 255s, the
 * result of each cell is its Manhattan distance to the nearest 0.)
 */
m.Map.prototype.expandInfluences = function(maximum = this.maxVal, grid = this.map)
{
	let w = this.width;
	let h = this.height;

	for (let y = 0; y < h; ++y)
	{
		let min = maximum;
		let x0 = y * w;
		for (let x = 0; x < w; ++x)
		{
			let g = grid[x + x0];
			if (g > min)
				grid[x + x0] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}

		for (let x = w - 2; x >= 0; --x)
		{
			let g = grid[x + x0];
			if (g > min)
				grid[x + x0] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}
	}

	for (let x = 0; x < w; ++x)
	{
		let min = maximum;
		for (let y = 0; y < h; ++y)
		{
			let g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}

		for (let y = h - 2; y >= 0; --y)
		{
			let g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}
	}
};

/** Multiplies current map by the parameter map pixelwise */
m.Map.prototype.multiply = function(map, onlyBetter, divider, maxMultiplier)
{
	for (let i = 0; i < this.length; ++i)
		if (map.map[i]/divider > 1)
			this.map[i] = Math.min(maxMultiplier*this.map[i], this.map[i] * (map.map[i]/divider));
};

/** add to current map by the parameter map pixelwise */
m.Map.prototype.add = function(map)
{
	for (let i = 0; i < this.length; ++i)
	{
		if (this.map[i] + map.map[i] < 0)
			this.map[i] = 0;
		else if (this.map[i] + map.map[i] > this.maxVal)
			this.map[i] = this.maxVal;
		else
			this.map[i] += map.map[i];
	}
};

/** Find the best non-obstructed tile */
m.Map.prototype.findBestTile = function(radius, obstruction)
{
	let bestIdx;
	let bestVal = -1;
	for (let j = 0; j < this.length; ++j)
	{
		if (this.map[j] <= bestVal)
			continue;
		let i = this.getNonObstructedTile(j, radius, obstruction);
		if (i < 0)
			continue;
		bestVal = this.map[j];
		bestIdx = i;
	}

	return [bestIdx, bestVal];
};

/** return any non obstructed (small) tile inside the (big) tile i from obstruction map */
m.Map.prototype.getNonObstructedTile = function(i, radius, obstruction)
{
	let ratio = this.cellSize / obstruction.cellSize;
	let ix = (i % this.width) * ratio;
	let iy = Math.floor(i / this.width) * ratio;
	let w = obstruction.width;
	for (let kx = ix; kx < ix + ratio; ++kx)
	{
		if (kx < radius || kx >= w - radius)
			continue;
		for (let ky = iy; ky < iy + ratio; ++ky)
		{
			if (ky < radius || ky >= w - radius)
				continue;
			if (obstruction.isObstructedTile(kx, ky, radius))
				continue;
			return kx + ky*w;
		}
	}
	return -1;
};

/** return true if the area centered on tile kx-ky and with radius is obstructed */
m.Map.prototype.isObstructedTile = function(kx, ky, radius)
{
	let w = this.width;
	if (kx < radius || kx >= w - radius || ky < radius || ky >= w - radius || this.map[kx+ky*w] === 0)
		return true;
	for (let dy = 0; dy <= radius; ++dy)
	{
		let dxmax = dy === 0 ? radius : Math.ceil(Math.sqrt(radius*radius - (dy-0.5)*(dy-0.5)));
		let xp = kx + (ky + dy)*w;
		let xm = kx + (ky - dy)*w;
		for (let dx = -dxmax; dx <= dxmax; ++dx)
			if (this.map[xp + dx] === 0 || this.map[xm + dx] === 0)
				return true;
	}
	return false;
};

/**
 * returns the nearest obstructed point
 * TODO check that the landpassmap index is the same
 */
m.Map.prototype.findNearestObstructed = function(k, radius)
{
	let w = this.width;
	let ix = k % w;
	let iy = Math.floor(k / w);
	let n = this.cellSize > 8 ? 1 : Math.floor(8 / this.cellSize);
	for (let i = 1; i <= n; ++i)
	{
		let kx = ix - i;
		let ky = iy + i;
		for (let j = 1; j <= 8*i; ++j)
		{
			if (this.isObstructedTile(kx, ky, radius))
			{
				let akx = Math.abs(kx-ix);
				let aky = Math.abs(ky-iy);
				if (akx >= aky)
				{
					if (kx > ix)
						--kx;
					else
						++kx;
				}
				if (aky >= akx)
				{
					if (ky > iy)
						--ky;
					else
						++ky;
				}
				return kx + w*ky;
			}
			if (j < 2*i+1)
				++kx;
			else if (j < 4*i+1)
				--ky;
			else if (j < 6*i+1)
				--kx;
			else
				++ky;
		}
	}
	return -1;
};

m.Map.prototype.dumpIm = function(name = "default.png", threshold = this.maxVal)
{
	Engine.DumpImage(name, this.map, this.width, this.height, threshold);
};

return m;

}(API3);
