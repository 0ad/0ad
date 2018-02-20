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
	let pathLength = this.start.distanceTo(this.end);

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

		segments1.push(Vector2D.sub(noiseStart, Vector2D.mult(noisePerpendicular, taperedWidth)).round());
		segments2.push(Vector2D.add(noiseEnd, Vector2D.mult(noisePerpendicular, taperedWidth)).round());
	}

	// Draw path segments
	let size = g_Map.getSize();
	let gotRet = new Array(size).fill(0).map(i => new Uint8Array(size));
	let retVec = [];
	let failed = 0;

	for (let j = 0; j < segments1.length - 1; ++j)
	{
		let points = new ConvexPolygonPlacer([segments1[j], segments1[j + 1], segments2[j], segments2[j + 1]], Infinity).place(new NullConstraint());
		if (!points)
			continue;

		for (let point of points)
		{
			if (!constraint.allows(point))
			{
				++failed;
				continue;
			}

			if (g_Map.inMapBounds(point) && !gotRet[point.x][point.y])
			{
				retVec.push(point);
				gotRet[point.x][point.y] = 1;
			}
		}
	}

	return failed > this.failFraction * this.width * pathLength ? undefined : retVec;
};
