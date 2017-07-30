/////////////////////////////////////////////////////////////////////////////////////////
//	ClumpPlacer
//
//	Class for generating a roughly circular clump of points
//
//	size: The average number of points in the clump
//	coherence: How much the radius of the clump varies (1.0 = circle, 0.0 = very random)
//	smoothness: How smooth the border of the clump is (1.0 = few "peaks", 0.0 = very jagged)
//	failfraction: Percentage of place attempts allowed to fail (optional)
//	x, z: Tile coordinates of placer center (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ClumpPlacer(size, coherence, smoothness, failFraction, x, z)
{
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.failFraction = failFraction !== undefined ? failFraction : 0;
	this.x = x !== undefined ? x : -1;
	this.z = z !== undefined ? z : -1;
}

ClumpPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(this.x, this.z))
		return undefined;

	var retVec = [];

	var size = getMapSize();
	var gotRet = new Array(size).fill(0).map(p => new Uint8Array(size)); // booleans
	var radius = sqrt(this.size / PI);
	var perim = 4 * radius * 2 * PI;
	var intPerim = ceil(perim);

	var ctrlPts = 1 + Math.floor(1.0/Math.max(this.smoothness,1.0/intPerim));

	if (ctrlPts > radius * 2 * PI)
		ctrlPts = Math.floor(radius * 2 * PI) + 1;

	var noise = new Float32Array(intPerim);			//float32
	var ctrlCoords = new Float32Array(ctrlPts+1);	//float32
	var ctrlVals = new Float32Array(ctrlPts+1);		//float32

	// Generate some interpolated noise
	for (var i=0; i < ctrlPts; i++)
	{
		ctrlCoords[i] = i * perim / ctrlPts;
		ctrlVals[i] = randFloat(0, 2);
	}

	var c = 0;
	var looped = 0;
	for (var i=0; i < intPerim; i++)
	{
		if (ctrlCoords[(c+1) % ctrlPts] < i && !looped)
		{
			c = (c+1) % ctrlPts;
			if (c == ctrlPts-1)
				looped = 1;
		}

		// Cubic interpolation of ctrlVals
		var t = (i - ctrlCoords[c]) / ((looped ? perim : ctrlCoords[(c+1)%ctrlPts]) - ctrlCoords[c]);
		var v0 = ctrlVals[(c+ctrlPts-1)%ctrlPts];
		var v1 = ctrlVals[c];
		var v2 = ctrlVals[(c+1)%ctrlPts];
		var v3 = ctrlVals[(c+2)%ctrlPts];
		var P = (v3 - v2) - (v0 - v1);
		var Q = v0 - v1 - P;
		var R = v2 - v0;
		var S = v1;

		noise[i] = P*t*t*t + Q*t*t + R*t + S;
	}

	var failed = 0;
	for (var p=0; p < intPerim; p++)
	{
		var th = 2 * PI * p / perim;
		var r = radius * (1 + (1-this.coherence)*noise[p]);
		var s = sin(th);
		var c = cos(th);
		var xx = this.x;
		var yy = this.z;

		for (var k=0; k < ceil(r); k++)
		{
			var i = Math.floor(xx);
			var j = Math.floor(yy);
			if (g_Map.inMapBounds(i, j) && constraint.allows(i, j))
			{
				if (!gotRet[i][j])
				{
					// Only include each point once
					gotRet[i][j] = 1;
					retVec.push(new PointXZ(i, j));
				}
			}
			else
				failed++;

			xx += s;
			yy += c;
		}
	}

	return failed > this.size * this.failFraction ? undefined : retVec;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	Chain Placer
//
//	Class for generating a more random clump of points it randomly creates circles around the edges of the current clump
//
//	minRadius: minimum radius of the circles
//	maxRadius: maximum radius of the circles
//	numCircles: the number of the circles
//	failfraction: Percentage of place attempts allowed to fail (optional)
//	x, z: Tile coordinates of placer center (optional)
//  fcc: Farthest circle center (optional)
//  q: a list containing numbers. each time if the list still contains values, pops one from the end and uses it as the radius (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function ChainPlacer(minRadius, maxRadius, numCircles, failFraction, x, z, fcc, q)
{
	this.minRadius = minRadius;
	this.maxRadius = maxRadius;
	this.numCircles = numCircles;
	this.failFraction = failFraction !== undefined ? failFraction : 0;
	this.x = x !== undefined ? x : -1;
	this.z = z !== undefined ? z : -1;
	this.fcc = fcc !== undefined ? fcc : 0;
	this.q = q !== undefined ? q : [];
}

ChainPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(this.x, this.z))
		return undefined;

	var retVec = [];
	var size = getMapSize();
	var failed = 0, count = 0;
	var queueEmpty = !this.q.length;

	var gotRet = new Array(size).fill(0).map(p => new Array(size).fill(-1));
	--size;

	this.minRadius = Math.min(this.maxRadius, Math.max(this.minRadius, 1));

	var edges = [[this.x, this.z]];
	for (var i = 0; i < this.numCircles; ++i)
	{
		var [cx, cz] = pickRandom(edges);
		if (queueEmpty)
			var radius = randIntInclusive(this.minRadius, this.maxRadius);
		else
		{
			var radius = this.q.pop();
			queueEmpty = !this.q.length;
		}

		var sx = cx - radius, lx = cx + radius;
		var sz = cz - radius, lz = cz + radius;

		sx = Math.max(0, sx);
		sz = Math.max(0, sz);
		lx = Math.min(lx, size);
		lz = Math.min(lz, size);

		var radius2 = radius * radius;
		var dx, dz;

		for (var ix = sx; ix <= lx; ++ix)
			for (var iz = sz; iz <= lz; ++ iz)
			{
				dx = ix - cx;
				dz = iz - cz;
				if (dx * dx + dz * dz <= radius2)
				{
					if (g_Map.inMapBounds(ix, iz) && constraint.allows(ix, iz))
					{
						var state = gotRet[ix][iz];
						if (state == -1)
						{
							retVec.push(new PointXZ(ix, iz));
							gotRet[ix][iz] = -2;
						}
						else if (state >= 0)
						{
							var s = edges.splice(state, 1);
							gotRet[ix][iz] = -2;

							var edgesLength = edges.length;
							for (var k = state; k < edges.length; ++k)
								--gotRet[edges[k][0]][edges[k][1]];
						}
					}
					else
						++failed;

					++count;
				}
			}

		for (var ix = sx; ix <= lx; ++ix)
			for (var iz = sz; iz <= lz; ++ iz)
			{
				if (this.fcc)
					if ((this.x - ix) > this.fcc || (ix - this.x) > this.fcc || (this.z - iz) > this.fcc || (iz - this.z) > this.fcc)
						continue;

				if (gotRet[ix][iz] == -2)
				{
					if (ix > 0)
					{
						if (gotRet[ix-1][iz] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (iz > 0)
					{
						if (gotRet[ix][iz-1] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (ix < size)
					{
						if (gotRet[ix+1][iz] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
					if (iz < size)
					{
						if (gotRet[ix][iz+1] == -1)
						{
							edges.push([ix, iz]);
							gotRet[ix][iz] = edges.length - 1;
							continue;
						}
					}
				}
			}
	}


	return failed > count * this.failFraction ? undefined : retVec;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	RectPlacer
//
//	Class for generating a rectangular block of points
//
//	x1,z1: Top left corner of block
//	x2,z2: Bottom right corner of block
//
/////////////////////////////////////////////////////////////////////////////////////////

function RectPlacer(x1, z1, x2, z2)
{
	this.x1 = x1;
	this.z1 = z1;
	this.x2 = x2;
	this.z2 = z2;

	if (x1 > x2 || z1 > z2)
		throw("RectPlacer: incorrect bounds on rect");
}

RectPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x1, this.z1) || !constraint.allows(this.x1, this.z1) ||
		!g_Map.inMapBounds(this.x2, this.z2) || !constraint.allows(this.x2, this.z2))
		return undefined;

	var ret = [];
	var x2 = this.x2;
	var z2 = this.z2;

	for (var x=this.x1; x < x2; x++)
		for (var z=this.z1; z < z2; z++)
			if (g_Map.inMapBounds(x, z) && constraint.allows(x, z))
				ret.push(new PointXZ(x, z));
			else
				return undefined;

	return ret;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	ObjectGroupPlacer
/////////////////////////////////////////////////////////////////////////////////////////

function ObjectGroupPlacer() {}

/////////////////////////////////////////////////////////////////////////////////////////
//	SimpleObject
//
//	Class specifying a type of entity that can be placed on the map
//
//	type: The entity's template name
//	minCount,maxCount: The number of objects to place
//	minDistance,maxDistance: The distance between placed objects
//	minAngle,maxAngle: The variation in angle of placed objects (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function SimpleObject(type, minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle)
{
	this.type = type;
	this.minCount = minCount;
	this.maxCount = maxCount;
	this.minDistance = minDistance;
	this.maxDistance = maxDistance;
	this.minAngle = minAngle !== undefined ? minAngle : 0;
	this.maxAngle = maxAngle !== undefined ? maxAngle : 2*PI;

	if (minCount > maxCount)
		warn("SimpleObject: minCount should be less than or equal to maxCount");

	if (minDistance > maxDistance)
		warn("SimpleObject: minDistance should be less than or equal to maxDistance");

	if (minAngle > maxAngle)
		warn("SimpleObject: minAngle should be less than or equal to maxAngle");
}

SimpleObject.prototype.place = function(cx, cz, player, avoidSelf, constraint, maxFailCount = 20)
{
	var failCount = 0;
	var resultObjs = [];

	for (var i = 0; i < randIntInclusive(this.minCount, this.maxCount); ++i)
		while(true)
		{
			var distance = randFloat(this.minDistance, this.maxDistance);
			var direction = randFloat(0, 2*PI);

			var x = cx + 0.5 + distance*cos(direction);
			var z = cz + 0.5 + distance*sin(direction);
			var fail = false; // reset place failure flag

			if (!g_Map.validT(x, z))
				fail = true;
			else
			{
				if (avoidSelf)
				{
					var length = resultObjs.length;
					for (var i = 0; (i < length) && !fail; i++)
					{
						var dx = x - resultObjs[i].position.x;
						var dy = z - resultObjs[i].position.z;

						if (dx*dx + dy*dy < 1)
							fail = true;
					}
				}

				if (!fail)
				{
					if (!constraint.allows(Math.floor(x), Math.floor(z)))
						fail = true;
					else
					{
						var angle = randFloat(this.minAngle, this.maxAngle);
						resultObjs.push(new Entity(this.type, player, x, z, angle));
						break;
					}
				}
			}

			if (fail)
			{
				failCount++;
				if (failCount > maxFailCount)
					return undefined;
			}
		}

	return resultObjs;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	RandomObject
//
//	Class specifying entities that can be placed on the map, selected randomly
//
//	types: Array of entity template names
//	minCount,maxCount: The number of objects to place
//	minDistance,maxDistance: The distance between placed objects
//	minAngle,maxAngle: The variation in angle of placed objects (optional)
//
/////////////////////////////////////////////////////////////////////////////////////////

function RandomObject(types, minCount, maxCount, minDistance, maxDistance, minAngle, maxAngle)
{
	this.types = types;
	this.minCount = minCount;
	this.maxCount = maxCount;
	this.minDistance = minDistance;
	this.maxDistance = maxDistance;
	this.minAngle = minAngle !== undefined ? minAngle : 0;
	this.maxAngle = maxAngle !== undefined ? maxAngle : 2*PI;

	if (minCount > maxCount)
		warn("RandomObject: minCount should be less than or equal to maxCount");

	if (minDistance > maxDistance)
		warn("RandomObject: minDistance should be less than or equal to maxDistance");

	if (minAngle > maxAngle)
		warn("RandomObject: minAngle should be less than or equal to maxAngle");
}

RandomObject.prototype.place = function(cx, cz, player, avoidSelf, constraint, maxFailCount = 20)
{
	var failCount = 0;
	var resultObjs = [];

	for (var i = 0; i < randIntInclusive(this.minCount, this.maxCount); ++i)
		while(true)
		{
			var distance = randFloat(this.minDistance, this.maxDistance);
			var direction = randFloat(0, 2*PI);

			var x = cx + 0.5 + distance*cos(direction);
			var z = cz + 0.5 + distance*sin(direction);
			var fail = false; // reset place failure flag

			if (!g_Map.validT(x, z))
				fail = true;
			else
			{
				if (avoidSelf)
				{
					var length = resultObjs.length;
					for (var i = 0; (i < length) && !fail; i++)
					{
						var dx = x - resultObjs[i].position.x;
						var dy = z - resultObjs[i].position.z;

						if (dx*dx + dy*dy < 1)
							fail = true;
					}
				}

				if (!fail)
				{
					if (!constraint.allows(Math.floor(x), Math.floor(z)))
						fail = true;
					else
					{
						var angle = randFloat(this.minAngle, this.maxAngle);
						resultObjs.push(new Entity(pickRandom(this.types), player, x, z, angle));
						break;
					}
				}
			}

			if (fail)
			{
				failCount++;
				if (failCount > maxFailCount)
					return undefined;
			}
		}

	return resultObjs;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	SimpleGroup
//
//	Class for placing groups of different objects
//
//	elements: Array of SimpleObjects
//	avoidSelf: Objects will not overlap
//	tileClass: Optional tile class to add with these objects
//	x,z: Tile coordinates of center of placer
//
/////////////////////////////////////////////////////////////////////////////////////////

function SimpleGroup(elements, avoidSelf, tileClass, x, z)
{
	this.elements = elements;
	this.tileClass = tileClass !== undefined ? getTileClass(tileClass) : undefined;
	this.avoidSelf = avoidSelf !== undefined ? avoidSelf : false;
	this.x = x !== undefined ? x : -1;
	this.z = z !== undefined ? z : -1;
}

SimpleGroup.prototype.place = function(player, constraint)
{
	var resultObjs = [];

	// Try placement of objects
	for (let element of this.elements)
	{
		var objs = element.place(this.x, this.z, player, this.avoidSelf, constraint);

		if (objs === undefined)
			return undefined;

		resultObjs = resultObjs.concat(objs);
	}

	// Add placed objects to map
	for (let obj of resultObjs)
	{
		if (g_Map.validT(obj.position.x / CELL_SIZE, obj.position.z / CELL_SIZE, MAP_BORDER_WIDTH))
			g_Map.addObject(obj);

		// Convert position to integer number of tiles
		if (this.tileClass !== undefined)
			this.tileClass.add(Math.floor(obj.position.x/CELL_SIZE), Math.floor(obj.position.z/CELL_SIZE));
	}

	return resultObjs;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	RandomGroup
//
//	Class for placing group of a random simple object
//
//	elements: Array of SimpleObjects
//	avoidSelf: Objects will not overlap
//	tileClass: Optional tile class to add with these objects
//	x,z: Tile coordinates of center of placer
//
/////////////////////////////////////////////////////////////////////////////////////////

function RandomGroup(elements, avoidSelf, tileClass, x, z)
{
	this.elements = elements;
	this.tileClass = tileClass !== undefined ? getTileClass(tileClass) : undefined;
	this.avoidSelf = avoidSelf !== undefined ? avoidSelf : false;
	this.x = x !== undefined ? x : -1;
	this.z = z !== undefined ? z : -1;
}

RandomGroup.prototype.place = function(player, constraint)
{
	var resultObjs = pickRandom(this.elements).place(this.x, this.z, player, this.avoidSelf, constraint);
	if (resultObjs === undefined)
		return undefined;

	// Add placed objects to map
	for (let obj of resultObjs)
	{
		g_Map.addObject(obj);

		// Convert position to integer number of tiles
		if (this.tileClass !== undefined)
			this.tileClass.add(Math.floor(obj.position.x/CELL_SIZE), Math.floor(obj.position.z/CELL_SIZE));
	}

	return resultObjs;
};
