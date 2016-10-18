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
	this.failFraction = (failFraction !== undefined ? failFraction : 0);
	this.x = (x !== undefined ? x : -1);
	this.z = (z !== undefined ? z : -1);
}

ClumpPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(this.x, this.z))
	{
		return undefined;
	}

	var retVec = [];
	
	var size = getMapSize();
	var gotRet = new Array(size);
	for (var i = 0; i < size; ++i)
	{
		gotRet[i] = new Uint8Array(size);			// bool / uint8
	}
	
	var radius = sqrt(this.size / PI);
	var perim = 4 * radius * 2 * PI;
	var intPerim = ceil(perim);
	
	var ctrlPts = 1 + Math.floor(1.0/Math.max(this.smoothness,1.0/intPerim));
	
	if (ctrlPts > radius * 2 * PI)
	{
		ctrlPts = Math.floor(radius * 2 * PI) + 1;
	}
	
	var noise = new Float32Array(intPerim);			//float32
	var ctrlCoords = new Float32Array(ctrlPts+1);	//float32
	var ctrlVals = new Float32Array(ctrlPts+1);		//float32

	// Generate some interpolated noise
	for (var i=0; i < ctrlPts; i++)
	{
		ctrlCoords[i] = i * perim / ctrlPts;
		ctrlVals[i] = 2.0*randFloat();
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
		var Q = (v0 - v1) - P;
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
		var xx=this.x;
		var yy=this.z;
		
		for (var k=0; k < ceil(r); k++)
		{
			var i = Math.floor(xx);
			var j = Math.floor(yy);
			if (g_Map.inMapBounds(i, j) && constraint.allows(i, j))
			{
				if (!gotRet[i][j])
				{	// Only include each point once
					gotRet[i][j] = 1;
					retVec.push(new PointXZ(i, j));
				}
			}
			else
			{
				failed++;
			}
			xx += s;
			yy += c;
		}
	}
	
	return ((failed > this.size*this.failFraction) ? undefined : retVec);
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
	this.failFraction = (failFraction !== undefined ? failFraction : 0);
	this.x = (x !== undefined ? x : -1);
	this.z = (z !== undefined ? z : -1);
	this.fcc = (fcc !== undefined ? fcc : 0);
	this.q = (q !== undefined ? q : []);
}

ChainPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x, this.z) || !constraint.allows(this.x, this.z))
	{
		return undefined;
	}

	var retVec = [];
	var size = getMapSize();
	var failed = 0, count = 0;
	var queueEmpty = (this.q.length ? false : true);
	
	var gotRet = new Array(size);
	for (var i = 0; i < size; ++i)
	{
		gotRet[i] = new Array(size);
		for (var j = 0; j < size; ++j)
		{
			gotRet[i][j] = -1;
		}
	}
	
	--size;
	
	if (this.minRadius < 1) this.minRadius = 1;
	if (this.minRadius > this.maxRadius) this.minRadius = this.maxRadius;
	
	var edges = [[this.x, this.z]];
	
	for (var i = 0; i < this.numCircles; ++i)
	{
		var point = edges[randInt(edges.length)];
		var cx = point[0], cz = point[1];
	
		if (queueEmpty)
		{
			var radius = randInt(this.minRadius, this.maxRadius);
		}
		else
		{
			var radius = this.q.pop();
			queueEmpty = (this.q.length ? false : true);
		}
		
		//log (edges);
		
		var sx = cx - radius, lx = cx + radius;
		var sz = cz - radius, lz = cz + radius;
		
		sx = (sx < 0 ? 0 : sx);
		sz = (sz < 0 ? 0 : sz);
		lx = (lx > size ? size : lx);
		lz = (lz > size ? size : lz);
		
		var radius2 = radius * radius;
		var dx, dz;
		
		//log (uneval([sx, sz, lx, lz]));
		
		for (var ix = sx; ix <= lx; ++ix)
		{
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
							//log (uneval(edges));
							//log (state)
							var s = edges.splice(state, 1);
							//log (uneval(s));
							//log (uneval(edges));
							gotRet[ix][iz] = -2;
							
							var edgesLength = edges.length;
							for (var k = state; k < edges.length; ++k)
							{
								--gotRet[edges[k][0]][edges[k][1]];
							}
						}
					}
					else
					{
						++failed;
					}
					++count;
				}
			}
		}
		
		for (var ix = sx; ix <= lx; ++ix)
		{
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
		
	}
	
	
	return ((failed > count*this.failFraction) ? undefined : retVec);
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
	{
		throw("RectPlacer: incorrect bounds on rect");
	}
}

RectPlacer.prototype.place = function(constraint)
{
	// Preliminary bounds check
	if (!g_Map.inMapBounds(this.x1, this.z1) || !constraint.allows(this.x1, this.z1) ||
		!g_Map.inMapBounds(this.x2, this.z2) || !constraint.allows(this.x2, this.z2))
	{
		return undefined;
	}

	var ret = [];
	
	var x2 = this.x2;
	var z2 = this.z2;
	
	for (var x=this.x1; x < x2; x++)
	{
		for (var z=this.z1; z < z2; z++)
		{
			if (g_Map.inMapBounds(x, z) && constraint.allows(x, z))
			{
				ret.push(new PointXZ(x, z));
			}
			else
			{
				return undefined;
			}
		}
	}
	
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
	this.minAngle = (minAngle !== undefined ? minAngle : 0);
	this.maxAngle = (maxAngle !== undefined ? maxAngle : 2*PI);
	
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
	var count = randInt(this.minCount, this.maxCount);
	var resultObjs = [];
	
	for (var i=0; i < count; i++)
	{
		while(true)
		{
			var distance = randFloat(this.minDistance, this.maxDistance);
			var direction = randFloat(0, 2*PI);

			var x = cx + 0.5 + distance*cos(direction);
			var z = cz + 0.5 + distance*sin(direction);
			var fail = false; // reset place failure flag

			if (!g_Map.validT(x, z))
			{
				fail = true;
			}
			else
			{
				if (avoidSelf)
				{
					var length = resultObjs.length;
					for (var i = 0; (i < length) && !fail; i++)
					{
						var dx = x - resultObjs[i].position.x;
						var dy = z - resultObjs[i].position.z;
						
						if ((dx*dx + dy*dy) < 1)
						{
							fail = true;
						}
					}
				}
				
				if (!fail)
				{
					if (!constraint.allows(Math.floor(x), Math.floor(z)))
					{
						fail = true;
					}
					else
					{	// if we got here, we're good
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
	this.minAngle = (minAngle !== undefined ? minAngle : 0);
	this.maxAngle = (maxAngle !== undefined ? maxAngle : 2*PI);
	
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
	var count = randInt(this.minCount, this.maxCount);
	var resultObjs = [];
	
	for (var i=0; i < count; i++)
	{
		while(true)
		{
			var distance = randFloat(this.minDistance, this.maxDistance);
			var direction = randFloat(0, 2*PI);

			var x = cx + 0.5 + distance*cos(direction);
			var z = cz + 0.5 + distance*sin(direction);
			var fail = false; // reset place failure flag

			if (!g_Map.validT(x, z))
			{
				fail = true;
			}
			else
			{
				if (avoidSelf)
				{
					var length = resultObjs.length;
					for (var i = 0; (i < length) && !fail; i++)
					{
						var dx = x - resultObjs[i].position.x;
						var dy = z - resultObjs[i].position.z;
						
						if ((dx*dx + dy*dy) < 1)
						{
							fail = true;
						}
					}
				}
				
				if (!fail)
				{
					if (!constraint.allows(Math.floor(x), Math.floor(z)))
					{
						fail = true;
					}
					else
					{	// if we got here, we're good
						var angle = randFloat(this.minAngle, this.maxAngle);
						
						//Randomly select entity
						var type = this.types[randInt(this.types.length)];
						resultObjs.push(new Entity(type, player, x, z, angle));
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
	this.tileClass = (tileClass !== undefined ? getTileClass(tileClass) : undefined);
	this.avoidSelf = (avoidSelf !== undefined ? avoidSelf : false);
	this.x = (x !== undefined ? x : -1);
	this.z = (z !== undefined ? z : -1);
}

SimpleGroup.prototype.place = function(player, constraint)
{
	var resultObjs = [];

	// Try placement of objects
	var length = this.elements.length;
	for (var i = 0; i < length; i++)
	{
		var objs = this.elements[i].place(this.x, this.z, player, this.avoidSelf, constraint);
		if (objs === undefined)
		{	// Failure
			return false;
		}
		else
		{
			for (var j = 0; j < objs.length; ++j)
				resultObjs.push(objs[j]);
		}
	}
	
	// Add placed objects to map
	length = resultObjs.length;
	for (var i=0; i < length; i++)
	{
		if (g_Map.validT(resultObjs[i].position.x / CELL_SIZE, resultObjs[i].position.z / CELL_SIZE, MAP_BORDER_WIDTH))
			g_Map.addObject(resultObjs[i]);
		
		// Convert position to integer number of tiles
		if (this.tileClass !== undefined)
			this.tileClass.add(Math.floor(resultObjs[i].position.x/CELL_SIZE), Math.floor(resultObjs[i].position.z/CELL_SIZE));
	}
	
	return true;
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
	this.tileClass = (tileClass !== undefined ? getTileClass(tileClass) : undefined);
	this.avoidSelf = (avoidSelf !== undefined ? avoidSelf : false);
	this.x = (x !== undefined ? x : -1);
	this.z = (z !== undefined ? z : -1);
}

RandomGroup.prototype.place = function(player, constraint)
{
	var resultObjs = [];

	// Pick one of the object placers at random
	var placer = this.elements[randInt(this.elements.length)];
	
	var objs = placer.place(this.x, this.z, player, this.avoidSelf, constraint);
	// Failure
	if (objs === undefined)
	{
		return false;
	}
	else
	{
		for (var j = 0; j < objs.length; ++j)
			resultObjs.push(objs[j]);
	}
	
	// Add placed objects to map
	var length = resultObjs.length;
	for (var i=0; i < length; i++)
	{
		g_Map.addObject(resultObjs[i]);
		
		// Convert position to integer number of tiles
		if (this.tileClass !== undefined)
			this.tileClass.add(Math.floor(resultObjs[i].position.x/CELL_SIZE), Math.floor(resultObjs[i].position.z/CELL_SIZE));
	}
	
	return true;
};
