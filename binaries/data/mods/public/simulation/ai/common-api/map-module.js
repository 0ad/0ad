var API3 = function(m)
{

/* The map module.
 * Copied with changes from QuantumState's original for qBot, it's a component for storing 8 bit values.
 */

// The function needs to be named too because of the copyConstructor functionality
m.Map = function Map(sharedScript, originalMap, actualCopy){
	// get the map to find out the correct dimensions
	var gameMap = sharedScript.passabilityMap;
	this.width = gameMap.width;
	this.height = gameMap.height;
	this.length = gameMap.data.length;
	
	this.maxVal = 255;

	if (originalMap && actualCopy){
		this.map = new Uint8Array(this.length);
		for (var i = 0; i < originalMap.length; ++i)
			this.map[i] = originalMap[i];
	} else if (originalMap) {
		this.map = originalMap;
	} else {
		this.map = new Uint8Array(this.length);
	}
	this.cellSize = 4;
}

m.Map.prototype.setMaxVal = function(val){
	this.maxVal = val;
};

m.Map.prototype.gamePosToMapPos = function(p){
	return [Math.floor(p[0]/this.cellSize), Math.floor(p[1]/this.cellSize)];
};

m.Map.prototype.point = function(p){
	var q = this.gamePosToMapPos(p);
	q[0] = q[0] >= this.width ? this.width : (q[0] < 0 ? 0 : q[0]);
	q[1] = q[1] >= this.width ? this.width : (q[1] < 0 ? 0 : q[1]);
	return this.map[q[0] + this.width * q[1]];
};

m.Map.prototype.addInfluence = function(cx, cy, maxDist, strength, type) {
	strength = strength ? +strength : +maxDist;
	type = type ? type : 'linear';
	
	var x0 = Math.floor(Math.max(0, cx - maxDist));
	var y0 = Math.floor(Math.max(0, cy - maxDist));
	var x1 = Math.floor(Math.min(this.width-1, cx + maxDist));
	var y1 = Math.floor(Math.min(this.height-1, cy + maxDist));
	var maxDist2 = maxDist * maxDist;
	
	var str = 0.0;
	switch (type){
		case 'linear':
			str = +strength / +maxDist;
		break;
		case 'quadratic':
			str = +strength / +maxDist2;
		break;
		case 'constant':
			str = +strength;
		break;
	}
	
	// code duplicating for speed
	if (type === 'linear' || type === "linear")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2) {
					var quant = 0;
					var r = Math.sqrt(r2);
					quant = str * (maxDist - r);
					
					if (this.map[x + y * this.width] + quant < 0)
						this.map[x + y * this.width] = 0;
					else if (this.map[x + y * this.width] + quant > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;	// avoids overflow.
					else
						this.map[x + y * this.width] += quant;
				}
			}
		}
	} else if (type === 'quadratic' || type === "quadratic")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2){
					var quant = str * (maxDist2 - r2);
					
					if (this.map[x + y * this.width] + quant < 0)
						this.map[x + y * this.width] = 0;
					else if (this.map[x + y * this.width] + quant > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;	// avoids overflow.
					else
						this.map[x + y * this.width] += quant;
				}
			}
		}
	} else if (type === 'constant' || type === "constant")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2){
					if (this.map[x + y * this.width] + str < 0)
						this.map[x + y * this.width] = 0;
					else if (this.map[x + y * this.width] + str > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;	// avoids overflow.
					else
						this.map[x + y * this.width] += str;
				}
			}
		}
	}
};

m.Map.prototype.multiplyInfluence = function(cx, cy, maxDist, strength, type) {
	strength = strength ? +strength : +maxDist;
	type = type ? type : 'constant';
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	var str = 0.0;
	switch (type){
		case 'linear':
			str = strength / maxDist;
			break;
		case 'quadratic':
			str = strength / maxDist2;
			break;
		case 'constant':
			str = strength;
			break;
	}
	
	if (type === 'linear' || type === "linear")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2){
					var quant = 0;
					var r = Math.sqrt(r2);
					quant = str * (maxDist - r);
					
					var machin = this.map[x + y * this.width] * quant;
					if (machin < 0)
						this.map[x + y * this.width] = 0;
					else if (machin > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;
					else
						this.map[x + y * this.width] = machin;

				}
			}
		}
	} else if (type === 'quadratic' || type === "quadratic")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2){
					var quant = str * (maxDist2 - r2);
					
					var machin = this.map[x + y * this.width] * quant;
					if (machin < 0)
						this.map[x + y * this.width] = 0;
					else if (machin > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;
					else
						this.map[x + y * this.width] = machin;

				}
			}
		}
	} else if (type === 'constant' || type === "constant")
	{
		for ( var y = y0; y < y1; ++y) {
			for ( var x = x0; x < x1; ++x) {
				var dx = x - cx;
				var dy = y - cy;
				var r2 = dx*dx + dy*dy;
				if (r2 < maxDist2){
					var machin = this.map[x + y * this.width] * str;
					if (machin < 0)
						this.map[x + y * this.width] = 0;
					else if (machin > this.maxVal)
						this.map[x + y * this.width] = this.maxVal;
					else
						this.map[x + y * this.width] = machin;

				}
			}
		}
	}
};

// doesn't check for overflow.
m.Map.prototype.setInfluence = function(cx, cy, maxDist, value) {
	value = value ? value : 0;
	
	var x0 = Math.max(0, cx - maxDist);
	var y0 = Math.max(0, cy - maxDist);
	var x1 = Math.min(this.width, cx + maxDist);
	var y1 = Math.min(this.height, cy + maxDist);
	var maxDist2 = maxDist * maxDist;
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < maxDist2){
				this.map[x + y * this.width] = value;
			}
		}
	}
};

m.Map.prototype.sumInfluence = function(cx, cy, radius){
	var x0 = Math.max(0, cx - radius);
	var y0 = Math.max(0, cy - radius);
	var x1 = Math.min(this.width, cx + radius);
	var y1 = Math.min(this.height, cy + radius);
	var radius2 = radius * radius;
	
	var sum = 0;
	
	for ( var y = y0; y < y1; ++y) {
		for ( var x = x0; x < x1; ++x) {
			var dx = x - cx;
			var dy = y - cy;
			var r2 = dx*dx + dy*dy;
			if (r2 < radius2){
				sum += this.map[x + y * this.width];
			}
		}
	}
	return sum;
};
/**
 * Make each cell's 16-bit/8-bit value at least one greater than each of its
 * neighbours' values. (If the grid is initialised with 0s and 65535s or 255s, the
 * result of each cell is its Manhattan distance to the nearest 0.)
 */
m.Map.prototype.expandInfluences = function(maximum, map) {
	var grid = this.map;
	if (map !== undefined)
		grid = map;
	
	if (maximum == undefined)
		maximum = this.maxVal;
	var w = this.width;
	var h = this.height;
	for ( var y = 0; y < h; ++y) {
		var min = maximum;
		for ( var x = 0; x < w; ++x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}
		
		for ( var x = w - 2; x >= 0; --x) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}
	}
	
	for ( var x = 0; x < w; ++x) {
		var min = maximum;
		for ( var y = 0; y < h; ++y) {
			var g = grid[x + y * w];
			if (g > min)
				grid[x + y * w] = min;
			else if (g < min)
				min = g;
			++min;
			if (min > maximum)
				min = maximum;
		}
		
		for ( var y = h - 2; y >= 0; --y) {
			var g = grid[x + y * w];
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

// Multiplies current map by the parameter map pixelwise
m.Map.prototype.multiply = function(map, onlyBetter, divider, maxMultiplier){
	for (var i = 0; i < this.length; ++i){
		if (map.map[i]/divider > 1)
			this.map[i] = Math.min(maxMultiplier*this.map[i], this.map[i] * (map.map[i]/divider));
	}
};
// add to current map by the parameter map pixelwise
m.Map.prototype.add = function(map){
	for (var i = 0; i < this.length; ++i) {
		if (this.map[i] + map.map[i] < 0)
			this.map[i] = 0;
		else if (this.map[i] + map.map[i] > this.maxVal)
			this.map[i] = this.maxVal;
		else
			this.map[i] += map.map[i];
	}
};

m.Map.prototype.findBestTile = function(radius, obstructionTiles){
	// Find the best non-obstructed tile
	var bestIdx = 0;
	var bestVal = -1;
	for ( var i = 0; i < this.length; ++i) {
		if (obstructionTiles.map[i] > radius) {
			var v = this.map[i];
			if (v > bestVal) {
				bestVal = v;
				bestIdx = i;
			}
		}
	}
	
	return [bestIdx, bestVal];
};

// returns the point with the lowest radius in the immediate vicinity
m.Map.prototype.findLowestNeighbor = function(x,y) {
	var lowestPt = [0,0];
	var lowestcoeff = 99999;
	x = Math.floor(x/4);
	y = Math.floor(y/4);
	for (var xx = x-1; xx <= x+1; ++xx)
		for (var yy = y-1; yy <= y+1; ++yy)
			if (xx >= 0 && xx < this.width && yy >= 0 && yy < this.width)
				if (this.map[xx+yy*this.width] <= lowestcoeff)
				{
					lowestcoeff = this.map[xx+yy*this.width];
					lowestPt = [(xx+0.5)*4, (yy+0.5)*4];
				}
	return lowestPt;
}

m.Map.prototype.dumpIm = function(name, threshold){
	name = name ? name : "default.png";
	threshold = threshold ? threshold : this.maxVal;
	Engine.DumpImage(name, this.map, this.width, this.height, threshold);
};

return m;

}(API3);
