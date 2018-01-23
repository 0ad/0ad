/**
 * @file A Non-Centered Placer generates a shape (array of points) at a fixed location meeting a Constraint and
 * is typically called by createArea.
 * Since this type of Placer has no x and z property, its location cannot be randomized using createAreas.
 */

/**
 * The RectPlacer provides the points on the tilegrid between (x1, z1) and (x2, z2) that meet the Constraint.
 */
function RectPlacer(x1, z1, x2, z2)
{
	let mapSize = getMapSize();

	this.x1 = Math.round(Math.max(Math.min(x1, x2), 0));
	this.x2 = Math.round(Math.min(Math.max(x1, x2), mapSize - 1));

	this.z1 = Math.round(Math.max(Math.min(z1, z2), 0));
	this.z2 = Math.round(Math.min(Math.max(z1, z2), mapSize - 1));
}

RectPlacer.prototype.place = function(constraint)
{
	let points = [];

	for (let x = this.x1; x <= this.x2; ++x)
		for (let z = this.z1; z <= this.z2; ++z)
		{
			let position = new Vector2D(x, z);
			if (constraint.allows(x, z))
				points.push(position);
		}

	return points;
};

/**
 * The MapBoundsPlacer returns all points on the tilemap that meet the constraint.
 */
function MapBoundsPlacer()
{
	let mapBounds = getMapBounds();
	this.rectPlacer = new RectPlacer(mapBounds.left, mapBounds.top, mapBounds.right, mapBounds.bottom);
}

MapBoundsPlacer.prototype.place = function(constraint)
{
	return this.rectPlacer.place(constraint);
};

/**
 * HeightPlacer constants determining whether the extrema should be included by the placer too.
 */
const Elevation_ExcludeMin_ExcludeMax = 0;
const Elevation_IncludeMin_ExcludeMax = 1;
const Elevation_ExcludeMin_IncludeMax = 2;
const Elevation_IncludeMin_IncludeMax = 3;

/**
 * The HeightPlacer provides all points between the minimum and maximum elevation that meet the Constraint,
 * even if they are far from the passable area of the map.
 */
function HeightPlacer(mode, minElevation, maxElevation)
{
	this.withinHeightRange =
		mode == Elevation_ExcludeMin_ExcludeMax ? (x, z) => g_Map.height[x][z] >  minElevation && g_Map.height[x][z] < maxElevation :
		mode == Elevation_IncludeMin_ExcludeMax ? (x, z) => g_Map.height[x][z] >= minElevation && g_Map.height[x][z] < maxElevation :
		mode == Elevation_ExcludeMin_IncludeMax ? (x, z) => g_Map.height[x][z] >  minElevation && g_Map.height[x][z] <= maxElevation :
		mode == Elevation_IncludeMin_IncludeMax ? (x, z) => g_Map.height[x][z] >= minElevation && g_Map.height[x][z] <= maxElevation :
		undefined;

	if (!this.withinHeightRange)
		throw new Error("Invalid HeightPlacer mode: " + mode);
}

HeightPlacer.prototype.place = function(constraint)
{
	let mapSize = getMapSize();
	let points = [];

	for (let x = 0; x < mapSize; ++x)
		for (let z = 0; z < mapSize; ++z)
		{
			let position = new Vector2D(x, z);
			if (this.withinHeightRange(x, z) && constraint.allows(x, z))
				points.push(position);
		}

	return points;
};

/////////////////////////////////////////////////////////////////////////////////////////
//	PathPlacer
//
//	Class for creating a winding path between two points
//
//	x1,z1: 	Starting point of path
//	x2,z2: 	Ending point of path
//	width: 	Width of the path
//	a:		Waviness - how wavy the path will be (higher is wavier, 0.0 is straight line)
//	b:		Smoothness - how smooth the path will be (higher is smoother)
//	c:		Offset - max amplitude of waves along the path (0.0 is straight line)
// 	taper:	Tapering - how much the width of the path changes from start to end
//				if positive, the width will decrease by that factor, if negative the width
//				will increase by that factor
//
/////////////////////////////////////////////////////////////////////////////////////////

function PathPlacer(start, end, width, a, b, c, taper, failFraction = 0)
{
	this.start = start;
	this.end = end;
	this.width = width;
	this.a = a;
	this.b = b;
	this.c = c;
	this.taper = taper;
	this.failFraction = failFraction;
}

PathPlacer.prototype.place = function(constraint)
{
	var failed = 0;

	let pathLength = this.start.distanceTo(this.end);
	var numSteps = 1 + Math.floor(pathLength / 4 * this.a);
	var numISteps = 1 + Math.floor(pathLength / 4 * this.b);
	var totalSteps = numSteps*numISteps;
	var offset = 1 + Math.floor(pathLength / 4 * this.c);

	var size = getMapSize();
	var gotRet = [];
	for (var i = 0; i < size; ++i)
		gotRet[i] = new Uint8Array(size);			// bool / uint8

	// Generate random offsets
	var ctrlVals = new Float32Array(numSteps);		//float32
	for (var j = 1; j < (numSteps-1); ++j)
	{
		ctrlVals[j] = randFloat(-offset, offset);
	}

	// Interpolate for smoothed 1D noise
	var noise = new Float32Array(totalSteps+1);		//float32
	for (var j = 0; j < numSteps; ++j)
		for (let k = 0; k < numISteps; ++k)
			noise[j * numISteps + k] = cubicInterpolation(
				1,
				k / numISteps,
				ctrlVals[(j + numSteps - 1) % numSteps],
				ctrlVals[j],
				ctrlVals[(j + 1) % numSteps],
				ctrlVals[(j + 2) % numSteps]);

	// Add smoothed noise to straight path
	let pathPerpendicular = Vector2D.sub(this.end, this.start).normalize().perpendicular();
	var segments1 = [];
	var segments2 = [];

	for (var j = 0; j < totalSteps; ++j)
	{
		// Interpolated points along straight path
		let step1 = j / totalSteps;
		let step2 = (j + 1) / totalSteps;
		let stepStart = Vector2D.add(Vector2D.mult(this.start, 1 - step1), Vector2D.mult(this.end, step1));
		let stepEnd = Vector2D.add(Vector2D.mult(this.start, 1 - step2), Vector2D.mult(this.end, step2));

		// Find noise offset points
		let noiseStart = Vector2D.add(stepStart, Vector2D.mult(pathPerpendicular, noise[j]));
		let noiseEnd = Vector2D.add(stepEnd, Vector2D.mult(pathPerpendicular, noise[j + 1]));
		let noisePerpendicular = Vector2D.sub(noiseEnd, noiseStart).normalize().perpendicular();

		let taperedWidth = (1 - step1 * this.taper) * this.width / 2;

		let point1 = Vector2D.sub(noiseStart, Vector2D.mult(noisePerpendicular, taperedWidth)).round();
		let point2 = Vector2D.add(noiseEnd, Vector2D.mult(noisePerpendicular, taperedWidth)).round();

		segments1.push({ "x": point1.x, "z": point1.y });
		segments2.push({ "x": point2.x, "z": point2.y });
	}

	var retVec = [];
	// Draw path segments
	var num = segments1.length - 1;
	for (var j = 0; j < num; ++j)
	{
		// Fill quad formed by these 4 points
		// Note the code assumes these points have been rounded to integer values
		var pt11 = segments1[j];
		var pt12 = segments1[j+1];
		var pt21 = segments2[j];
		var pt22 = segments2[j+1];

		var tris = [[pt12, pt11, pt21], [pt12, pt21, pt22]];

		for (var t = 0; t < 2; ++t)
		{
			// Sort vertices by min z
			var tri = tris[t].sort(
				function(a, b)
				{
					return a.z - b.z;
				}
			);

			// Fills in a line from (z, x1) to (z,x2)
			var fillLine = function(z, x1, x2)
			{
				var left = Math.round(Math.min(x1, x2));
				var right = Math.round(Math.max(x1, x2));
				for (var x = left; x <= right; x++)
				{
					let position = new Vector2D(x, z);
					if (constraint.allows(x, z))
					{
						if (g_Map.inMapBounds(x, z) && !gotRet[x][z])
						{
							retVec.push(position);
							gotRet[x][z] = 1;
						}
					}
					else
						failed++;
				}
			};

			var A = tri[0];
			var B = tri[1];
			var C = tri[2];

			var dx1 = (B.z != A.z) ? ((B.x - A.x) / (B.z - A.z)) : 0;
			var dx2 = (C.z != A.z) ? ((C.x - A.x) / (C.z - A.z)) : 0;
			var dx3 = (C.z != B.z) ? ((C.x - B.x) / (C.z - B.z)) : 0;

			if (A.z == B.z)
			{
				fillLine(A.z, A.x, B.x);
			}
			else
			{
				for (var z = A.z; z <= B.z; z++)
				{
					fillLine(z, A.x + dx1*(z - A.z), A.x + dx2*(z - A.z));
				}
			}
			if (B.z == C.z)
			{
				fillLine(B.z, B.x, C.x);
			}
			else
			{
				for (var z = B.z + 1; z < C.z; z++)
				{
					fillLine(z, B.x + dx3*(z - B.z), A.x + dx2*(z - A.z));
				}
			}
		}
	}

	return failed > this.failFraction * this.width * pathLength ? undefined : retVec;
};

/**
 * Creates a winded path between the given two vectors.
 * Uses a random angle at each step, so it can be more random than the sin form of the PathPlacer.
 * Omits the given offset after the start and before the end.
 */
function RandomPathPlacer(pathStart, pathEnd, pathWidth, offset, blended)
{
	this.pathStart = Vector2D.add(pathStart, Vector2D.sub(pathEnd, pathStart).normalize().mult(offset)).round();
	this.pathEnd = pathEnd;
	this.offset = offset;
	this.blended = blended;
	this.clumpPlacer = new ClumpPlacer(diskArea(pathWidth), 1, 1, 1);
	this.maxPathLength = fractionToTiles(2);
}

RandomPathPlacer.prototype.place = function(constraint)
{
	let pathLength = 0;
	let points = [];
	let position = this.pathStart;

	while (position.distanceTo(this.pathEnd) >= this.offset && pathLength++ < this.maxPathLength)
	{
		position.add(
			new Vector2D(1, 0).rotate(
				-getAngle(this.pathStart.x, this.pathStart.y, this.pathEnd.x, this.pathEnd.y) +
				-Math.PI / 2 * (randFloat(-1, 1) + (this.blended ? 0.5 : 0)))).round();

		this.clumpPlacer.x = position.x;
		this.clumpPlacer.z = position.y;

		for (let point of this.clumpPlacer.place(constraint) || [])
			if (points.every(p => p.x != point.x || p.y != point.y))
				points.push(point);
	}

	return points;
};
