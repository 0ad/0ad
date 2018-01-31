/**
 * @file A Non-Centered Placer generates a shape (array of points) at a fixed location meeting a Constraint and
 * is typically called by createArea.
 * Since this type of Placer has no x and z property, its location cannot be randomized using createAreas.
 */

/**
 * The RectPlacer returns all tiles between the two given points that meet the Constraint.
 */
function RectPlacer(start, end, failFraction = Infinity)
{
	this.bounds = getBoundingBox([start, end]);
	this.failFraction = failFraction;
}

RectPlacer.prototype.place = function(constraint)
{
	let bboxPoints = getPointsInBoundingBox(this.bounds);
	let points = bboxPoints.filter(point => g_Map.inMapBounds(point) && constraint.allows(point));
	return (bboxPoints.length - points.length) / bboxPoints.length <= this.failFraction ? points : undefined;
};

/**
 * The MapBoundsPlacer returns all points on the tilemap that meet the constraint.
 */
function MapBoundsPlacer(failFraction = Infinity)
{
	let mapBounds = g_Map.getBounds();
	this.rectPlacer = new RectPlacer(new Vector2D(mapBounds.left, mapBounds.top), new Vector2D(mapBounds.right, mapBounds.bottom), failFraction);
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
		mode == Elevation_ExcludeMin_ExcludeMax ? position => g_Map.getHeight(position) >  minElevation && g_Map.getHeight(position) < maxElevation :
		mode == Elevation_IncludeMin_ExcludeMax ? position => g_Map.getHeight(position) >= minElevation && g_Map.getHeight(position) < maxElevation :
		mode == Elevation_ExcludeMin_IncludeMax ? position => g_Map.getHeight(position) >  minElevation && g_Map.getHeight(position) <= maxElevation :
		mode == Elevation_IncludeMin_IncludeMax ? position => g_Map.getHeight(position) >= minElevation && g_Map.getHeight(position) <= maxElevation :
		undefined;

	if (!this.withinHeightRange)
		throw new Error("Invalid HeightPlacer mode: " + mode);
}

HeightPlacer.prototype.place = function(constraint)
{
	let mapSize = g_Map.getSize();

	return getPointsInBoundingBox(getBoundingBox([new Vector2D(0, 0), new Vector2D(mapSize - 1, mapSize - 1)])).filter(
		point => this.withinHeightRange(point) && constraint.allows(point));
};

/**
 * Creates a winding path between two points.
 *
 * @param {Vector2D} start - Starting position of the path.
 * @param {Vector2D} end - Endposition of the path.
 * @param {number} width - Number of tiles between two sides of the path.
 * @param {number} waviness - 0 is a straight line, higher numbers are.
 * @param {number} smoothness - the higher the number, the smoother the path.
 * @param {number} offset - Maximum amplitude of waves along the path. 0 is straight line.
 * @param {number} tapering - How much the width of the path changes from start to end.
 *   If positive, the width will decrease by that factor.
 *   If negative the width will increase by that factor.
 */
function PathPlacer(start, end, width, waviness, smoothness, offset, tapering, failFraction = 0)
{
	this.start = start;
	this.end = end;
	this.width = width;
	this.waviness = waviness;
	this.smoothness = smoothness;
	this.offset = offset;
	this.tapering = tapering;
	this.failFraction = failFraction;
}

PathPlacer.prototype.place = function(constraint)
{
	var failed = 0;

	let pathLength = this.start.distanceTo(this.end);

	var size = g_Map.getSize();
	var gotRet = [];
	for (var i = 0; i < size; ++i)
		gotRet[i] = new Uint8Array(size);			// bool / uint8
	let numStepsWaviness = 1 + Math.floor(pathLength / 4 * this.waviness);
	let numStepsLength = 1 + Math.floor(pathLength / 4 * this.smoothness);
	let offset = 1 + Math.floor(pathLength / 4 * this.offset);

	// Generate random offsets
	let ctrlVals = new Float32Array(numStepsWaviness);
	for (let j = 1; j < numStepsWaviness - 1; ++j)
		ctrlVals[j] = randFloat(-offset, offset);

	// Interpolate for smoothed 1D noise
	let totalSteps = numStepsWaviness * numStepsLength;
	let noise = new Float32Array(totalSteps + 1);
	for (let j = 0; j < numStepsWaviness; ++j)
		for (let k = 0; k < numStepsLength; ++k)
			noise[j * numStepsLength + k] = cubicInterpolation(
				1,
				k / numStepsLength,
				ctrlVals[(j + numStepsWaviness - 1) % numStepsWaviness],
				ctrlVals[j],
				ctrlVals[(j + 1) % numStepsWaviness],
				ctrlVals[(j + 2) % numStepsWaviness]);

	// Add smoothed noise to straight path
	let pathPerpendicular = Vector2D.sub(this.end, this.start).normalize().perpendicular();
	let segments1 = [];
	let segments2 = [];

	for (let j = 0; j < totalSteps; ++j)
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

		let taperedWidth = (1 - step1 * this.tapering) * this.width / 2;

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
					if (constraint.allows(position))
					{
						if (g_Map.inMapBounds(position) && !gotRet[x][z])
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

		this.clumpPlacer.setCenterPosition(position);

		for (let point of this.clumpPlacer.place(constraint) || [])
			if (points.every(p => p.x != point.x || p.y != point.y))
				points.push(point);
	}

	return points;
};
