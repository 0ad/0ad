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
	let map = type == "territory" || type == "resource" ? sharedScript.territoryMap : sharedScript.passabilityMap;
	this.width = map.width;
	this.height = map.height;
	this.cellSize = map.cellSize;
	this.length = this.width * this.height;

	this.maxVal = 255;

	// sanity check
	if (originalMap && originalMap.length != this.length)
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
	q[0] = q[0] >= this.width ? this.width-1 : q[0] < 0 ? 0 : q[0];
	q[1] = q[1] >= this.width ? this.width-1 : q[1] < 0 ? 0 : q[1];
	return this.map[q[0] + this.width * q[1]];
};

m.Map.prototype.runLoop = function(x0, x1, y0, y1, cx, cy, maxDist2, func)
{
	for (let y = y0; y < y1; ++y)
	{
		const dy2 = (y - cy) * (y - cy);
		const yw = y * this.width;
		for (let x = x0; x < x1; ++x)
		{
			const dx = x - cx;
			const r2 = dx * dx + dy2;
			if (r2 >= maxDist2)
				continue;
			const w = x + yw;
			this.set(w, func(w, r2));
		}
	}
};

m.Map.prototype.addInfluence = function(cx, cy, maxDist, strength, type = "linear")
{
	strength = strength ? strength : maxDist;

	const x0 = Math.floor(Math.max(0, cx - maxDist));
	const y0 = Math.floor(Math.max(0, cy - maxDist));
	const x1 = Math.floor(Math.min(this.width-1, cx + maxDist));
	const y1 = Math.floor(Math.min(this.height-1, cy + maxDist));
	const maxDist2 = maxDist * maxDist;

	if (type == "linear")
	{
		const str = strength / maxDist;
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => this.map[w] + str * (maxDist - Math.sqrt(r2)));
	}
	else if (type == "quadratic")
	{
		const str = strength / maxDist2;
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => this.map[w] + str * (maxDist2 - r2));
	}
	else
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => this.map[w] + strength);

};

m.Map.prototype.multiplyInfluence = function(cx, cy, maxDist, strength, type = "constant")
{
	strength = strength ? +strength : +maxDist;

	const x0 = Math.max(0, cx - maxDist);
	const y0 = Math.max(0, cy - maxDist);
	const x1 = Math.min(this.width, cx + maxDist);
	const y1 = Math.min(this.height, cy + maxDist);
	const maxDist2 = maxDist * maxDist;

	if (type == "linear")
	{
		const str = strength / maxDist;
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => str * (maxDist - Math.sqrt(r2)) * this.map[w]);
	}
	else if (type == "quadratic")
	{
		const str = strength / maxDist2;
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => str * (maxDist2 - r2) * this.map[w]);
	}
	else
		this.runLoop(x0, x1, y0, y1, cx, cy, maxDist2, (w, r2) => this.map[w] * strength);
};

/** add to current map by the parameter map pixelwise */
m.Map.prototype.add = function(map)
{
	for (let i = 0; i < this.length; ++i)
		this.set(i, this.map[i] + map.map[i]);
};

/** Set the value taking overflow into account */
m.Map.prototype.set = function(i, value)
{
	this.map[i] = value < 0 ? 0 : value > this.maxVal ? this.maxVal : value;
};

/** Find the best non-obstructed tile */
m.Map.prototype.findBestTile = function(radius, obstruction)
{
	let bestIdx;
	let bestVal = 0;
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

	return { "idx": bestIdx, "val": bestVal };
};

/** return any non obstructed (small) tile inside the (big) tile i from obstruction map */
m.Map.prototype.getNonObstructedTile = function(i, radius, obstruction)
{
	let ratio = this.cellSize / obstruction.cellSize;
	let ix = (i % this.width) * ratio;
	let iy = Math.floor(i / this.width) * ratio;
	let w = obstruction.width;
	let r2 = radius * radius;
	let lastPoint;
	for (let kx = ix; kx < ix + ratio; ++kx)
	{
		if (kx < radius || kx >= w - radius)
			continue;
		for (let ky = iy; ky < iy + ratio; ++ky)
		{
			if (ky < radius || ky >= w - radius)
				continue;
			if (lastPoint && (kx - lastPoint.x)*(kx - lastPoint.x) + (ky - lastPoint.y)*(ky - lastPoint.y) < r2)
				continue;
			lastPoint = obstruction.isObstructedTile(kx, ky, radius);
			if (!lastPoint)
				return kx + ky*w;
		}
	}
	return -1;
};

/** return true if the area centered on tile kx-ky and with radius is obstructed */
m.Map.prototype.isObstructedTile = function(kx, ky, radius)
{
	let w = this.width;
	if (kx < radius || kx >= w - radius || ky < radius || ky >= w - radius || this.map[kx+ky*w] == 0)
		return { "x": kx, "y": ky };
	if (!this.pattern || this.pattern[0] != radius)
	{
		this.pattern = [radius];
		let r2 = radius * radius;
		for (let i = 1; i <= radius; ++i)
			this.pattern.push(Math.floor(Math.sqrt(r2 - (i-0.5)*(i-0.5)) + 0.5));
	}
	for (let dy = 0; dy <= radius; ++dy)
	{
		let dxmax = this.pattern[dy];
		let xp = kx + (ky + dy)*w;
		let xm = kx + (ky - dy)*w;
		for (let dx = 0; dx <= dxmax; ++dx)
		{
			if (this.map[xp + dx] == 0)
				return { "x": kx + dx, "y": ky + dy };
			if (this.map[xm + dx] == 0)
				return { "x": kx + dx, "y": ky - dy };
			if (this.map[xp - dx] == 0)
				return { "x": kx - dx, "y": ky + dy };
			if (this.map[xm - dx] == 0)
				return { "x": kx - dx, "y": ky - dy };
		}
	}
	return null;
};

m.Map.prototype.dumpIm = function(name = "default.png", threshold = this.maxVal)
{
	Engine.DumpImage(name, this.map, this.width, this.height, threshold);
};

return m;

}(API3);
