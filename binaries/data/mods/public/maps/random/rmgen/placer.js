
/////////////////////////////////////////////////////////////////////////////////////////
//	ClumpPlacer
/////////////////////////////////////////////////////////////////////////////////////////

function ClumpPlacer(size, coherence, smoothness, failFraction, x, y)
{
	this.size = size;
	this.coherence = coherence;
	this.smoothness = smoothness;
	this.failFraction = (failFraction !== undefined ? failFraction : 0);
	this.x = (x !== undefined ? x : -1);
	this.y = (y !== undefined ? y : -1);
}

ClumpPlacer.prototype.place = function(constraint)
{
	if (!g_Map.validT(this.x, this.y) || !constraint.allows(this.x, this.y))
	{
		return false;
	}

	var retVec = [];
	
	var size = getMapSize();
	var gotRet = new Array(size);
	for (var i = 0; i < size; ++i)
	{
		gotRet[i] = new Uint8Array(size);		// bool / uint8
	}
	
	var radius = sqrt(this.size / PI);
	var perim = 4 * radius * 2 * PI;
	var intPerim = ceil(perim);
	
	var ctrlPts = 1 + Math.floor(1.0/Math.max(this.smoothness,1.0/intPerim));
	
	if (ctrlPts > radius * 2 * PI)
		ctrlPts = Math.floor(radius * 2 * PI) + 1;	
	
	var noise = new Float32Array(intPerim);		//float32
	var ctrlCoords = new Float32Array(ctrlPts+1);	//float32
	var ctrlVals = new Float32Array(ctrlPts+1);	//float32
	
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
		var yy=this.y;
		
		for (var k=0; k < ceil(r); k++)
		{
			var i = Math.floor(xx);
			var j = Math.floor(yy);
			if (g_Map.validT(i, j) && constraint.allows(i, j))
			{
				if (!gotRet[i][j])
				{	// Only include each point once
					gotRet[i][j] = 1;
					retVec.push(new Point(i, j));
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
//	RectPlacer
/////////////////////////////////////////////////////////////////////////////////////////

function RectPlacer(x1, y1, x2, y2)
{
	this.x1 = x1;
	this.y1 = y1;
	this.x2 = x2;
	this.y2 = y2;
	
	if (x1 > x2 || y1 > y2)
		error("RectPlacer: incorrect bounds on rect");
}

RectPlacer.prototype.place = function(constraint)
{
	var ret = [];
	
	var x2 = this.x2;
	var y2 = this.y2;
	
	for (var x=this.x1; x < x2; x++)
	{
		for (var y=this.y1; y < y2; y++)
		{
			if (g_Map.validT(x, y) && constraint.allows(x, y))
			{
				ret.push(new Point(x, y));
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
//	SimpleGroup
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
		error("SimpleObject: minCount must be less than or equal to maxCount");
	
	if (minDistance > maxDistance)
		error("SimpleObject: minDistance must be less than or equal to maxDistance");
	
	if (minAngle > maxAngle)
		error("SimpleObject: minAngle must be less than or equal to maxAngle");
}

SimpleObject.prototype.place = function(cx, cy, player, avoidSelf, constraint)
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
			var y = cy + 0.5 + distance*sin(direction);
			var fail = false; // reset place failure flag

			if (x < 0 || y < 0 || x > g_Map.size || y > g_Map.size)
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
						var dx = x - resultObjs[i].x;
						var dy = y - resultObjs[i].y;
						
						if ((dx*dx + dy*dy) < 1)
						{
							fail = true;
						}
					}
				}
				
				if (!fail)
				{
					if (!constraint.allows(Math.floor(x), Math.floor(y)))
					{
						fail = true;
					}
					else
					{	// if we got here, we're good
						var angle = randFloat(this.minAngle, this.maxAngle);
						resultObjs.push(new Entity(this.type, player, x, y, angle));
						break;
					}
				}
			}
			
			if (fail)
			{
				failCount++;
				if (failCount > 20)	// TODO: Make this adjustable
				{
					return undefined;
				}
			}
		}
	}
	
	return resultObjs;
};

function SimpleGroup(elements, avoidSelf, tileClass, x, y)
{
	this.elements = elements;
	this.tileClass = (tileClass !== undefined ? getTileClass(tileClass) : undefined);
	this.avoidSelf = (avoidSelf !== undefined ? avoidSelf : false);
	this.x = (x !== undefined ? x : -1);
	this.y = (y !== undefined ? y : -1);
}

SimpleGroup.prototype.place = function(player, constraint)
{
	var resultObjs = [];

	// Try placement of objects
	var length = this.elements.length;
	for (var i=0; i < length; i++)
	{
		var objs = this.elements[i].place(this.x, this.y, player, this.avoidSelf, constraint);
		if (objs === undefined)
		{	// Failure
			return false;
		}
		else
		{
			resultObjs = resultObjs.concat(objs);
		}
	}
	
	// Add placed objects to map
	length = resultObjs.length;
	for (var i=0; i < length; i++)
	{
		g_Map.addObjects(resultObjs[i]);
		
		if (this.tileClass !== undefined)
		{	// Round object position to integer
			this.tileClass.add(Math.floor(resultObjs[i].x), Math.floor(resultObjs[i].y));
		}
	}
	
	return true;
};

