/**
 * Returns the wall piece entities needed to construct a wall between start.pos and end.pos. Assumes start.pos != end.pos.
 * The result is an array of objects, each one containing the following information about a single wall piece entity:
 *   - 'template': the template name of the entity
 *   - 'pos': position of the entity, as an object with keys 'x' and 'z'
 *   - 'angle': orientation of the entity, as an angle in radians
 *
 * All the pieces in the resulting array are ordered left-to-right (or right-to-left) as they appear in the physical wall.
 *
 * @param placementData Object that associates the wall piece template names with information about those kinds of pieces.
 *                        Expects placementData[templateName].templateData to contain the parsed template information about
 *                        the template whose filename is <i>templateName</i>.
 * @param wallSet Object that primarily holds the template names for the supported wall pieces in this set (under the
 *                  'templates' key), as well as the min and max allowed overlap factors (see GetWallSegmentsRec). Expected
 *                  to contain template names for keys "long" (long wall segment), "medium" (medium wall segment), "short"
 *                  (short wall segment), "tower" (intermediate tower between wall segments), "gate" (replacement for long
 *                  walls).
 * @param start Object holding the starting position of the wall. Must contain keys 'x' and 'z'.
 * @param end   Object holding the ending position of the wall. Must contains keys 'x' and 'z'.
 */
function GetWallPlacement(placementData, wallSet, start, end)
{
	let candidateSegments = ["long", "medium", "short"].map(size => ({
		"template": wallSet.templates[size],
		"len": placementData[wallSet.templates[size]].templateData.wallPiece.length
	}));

	let towerWidth = placementData[wallSet.templates.tower].templateData.wallPiece.length;

	let dir = {
		"x": end.pos.x - start.pos.x,
		"z": end.pos.z - start.pos.z
	};

	let len = Math.sqrt(dir.x * dir.x + dir.z * dir.z);

	// we'll need room for at least our starting and ending towers to fit next to eachother
	if (len <= towerWidth)
		return [];

	let placement = GetWallSegmentsRec(
		len,
		candidateSegments,
		wallSet.minTowerOverlap,
		wallSet.maxTowerOverlap,
		towerWidth,
		0, []
	);

	// TODO: make sure intermediate towers are spaced out far enough for their obstructions to not overlap, implying that
	// tower's wallpiece lengths should be > their obstruction width, which is undesirable because it prevents towers with
	// wide bases
	if (!placement)
	{
		error("No placement possible for distance=" +
			Math.round(len * 1000) / 1000.0 +
			", minOverlap=" + wallSet.minTowerOverlap +
			", maxOverlap=" + wallSet.maxTowerOverlap);

		return [];
	}

	// List of chosen candidate segments
	let placedEntities = placement.segments;

	// placement.r is the remaining distance to target without towers (must be <= (N-1) * towerWidth)
	let spacing = placement.r / (2 * placedEntities.length);

	let dirNormalized = { "x": dir.x / len, "z": dir.z / len };

	// Angle of this wall segment (relative to world-space X/Z axes)
	let angle = -Math.atan2(dir.z, dir.x);

	let progress = 0;
	let result = [];

	for (let i = 0; i < placedEntities.length; ++i)
	{
		let placedEntity = placedEntities[i];

		result.push({
			"template": placedEntity.template,
			"pos": {
				"x": start.pos.x + (progress + spacing + placedEntity.len/2) * dirNormalized.x,
				"z": start.pos.z + (progress + spacing + placedEntity.len/2) * dirNormalized.z
			},
			"angle": angle,
		});

		if (i < placedEntities.length - 1)
		{
			result.push({
				"template": wallSet.templates.tower,
				"pos": {
					"x": start.pos.x + (progress + placedEntity.len + 2 * spacing) * dirNormalized.x,
					"z": start.pos.z + (progress + placedEntity.len + 2 * spacing) * dirNormalized.z
				},
				"angle": angle,
			});
		}

		progress += placedEntity.len + 2 * spacing;
	}

	return result;
}

/**
 * Helper function for GetWallPlacement. Finds a list of wall segments and the corresponding remaining spacing/overlap
 * distance "r" that will suffice to construct a wall of the given distance. It is understood that two extra towers will
 * be placed centered at the starting and ending points of the wall.
 *
 * @param d Total distance between starting and ending points (constant throughout calls).
 * @param candidateSegments List of candidate segments (constant throughout calls). Should be ordered longer-to-shorter
 *                            for better execution speed.
 * @param minOverlap Minimum overlap factor (constant throughout calls). Must have a value between 0 (meaning walls are
 *                     not allowed to overlap towers) and 1 (meaning they're allowed to overlap towers entirely).
 *                     Must be <= maxOverlap.
 * @param maxOverlap Maximum overlap factor (constant throughout calls). Must have a value between 0 (meaning walls are
 *                     not allowed to overlap towers) and 1 (meaning they're allowed to overlap towers entirely).
 *                     Must be >= minOverlap.
 * @param t Length of a single tower (constant throughout calls). Acts as buffer space for wall segments (see comments).
 * @param distSoFar Sum of all the wall segments' lengths in 'segments'.
 * @param segments Current list of wall segments placed.
 */
function GetWallSegmentsRec(d, candidateSegments, minOverlap, maxOverlap, t, distSoFar, segments)
{
	// The idea is to find a number N of wall segments (excluding towers) so that the sum of their lengths adds up to a
	// value that is within certain bounds of the distance 'd' between the starting and ending points of the wall. This
	// creates either a positive or negative 'buffer' of space, that can be compensated for by spacing the wall segments
	// out away from each other, or inwards, overlapping each other. The spaces or overlaps can then be covered up by
	// placing towers on top of them. In this way, the same set of wall segments can be used to span a wider range of
	// target distances.
	//
	// In this function, it is understood that two extra towers will be placed centered at the starting and ending points.
	// They are allowed to contribute to the buffer space.
	//
	// The buffer space equals the difference between d and the sum of the lengths of all the wall segments, and is denoted
	// 'r' for 'remaining space'. Positive values of r mean that the walls will need to be spaced out, negative values of r
	// mean that they will need to overlap. Clearly, there are limits to how far wall segments can be spaced out or
	// overlapped, depending on how much 'buffer space' each tower provides, and how far 'into' towers the wall segments are
	// allowed to overlap.
	//
	// Let 't' signify the width of a tower. When there are N wall segments, then the maximum distance that can be covered
	// using only these walls (plus the towers covering up any gaps) is achieved when the walls and towers touch outer-border-
	// to-outer-border. Therefore, the maximum value of r is then given by:
	//
	//   rMax = t/2 + (N-1)*t + t/2
	//        = N*t
	//
	// where the two half-tower widths are buffer space contributed by the implied towers on the starting and ending points.
	// Similarly, a value rMin = -N*t can be derived for the minimal value of r. Note that a value of r = 0 means that the
	// wall segment lengths add up to exactly d, meaning that each one starts and ends right in the center of a tower.
	//
	// Thus, we establish:
	//   -Nt <= r <= Nt
	//
	// We can further generalize this by adding in parameters to control the depth to within which wall segments are allowed to
	// overlap with a tower. The bounds above assume that a wall segment is allowed to overlap across the entire range of 0
	// (not overlapping at all, as in the upper boundary) to 1 (overlapping maximally, as in the lower boundary).
	//
	// By requiring that walls overlap towers to a degree of at least 0 < minOverlap <= 1, it is clear that this lowers the
	// distance that can be maximally reached by the same set of wall segments, compared to the value of minOverlap = 0 that
	// we assumed to initially find Nt.
	//
	// Consider a value of minOverlap = 0.5, meaning that any wall segment must protrude at least halfway into towers; in this
	// situation, wall segments must at least touch boundaries or overlap mutually, implying that the sum of their lengths
	// must equal or exceed 'd', establishing an upper bound of 0 for r.
	// Similarly, consider a value of minOverlap = 1, meaning that any wall segment must overlap towers maximally; this situation
	// is equivalent to the one for finding the lower bound -Nt on r.
	//
	// With the implicit value minOverlap = 0 that yielded the upper bound Nt above, simple interpolation and a similar exercise
	// for maxOverlap, we find:
	//   (1-2*maxOverlap) * Nt <= r <= (1-2*minOverlap) * Nt
	//
	// To find N segments that satisfy this requirement, we try placing L, M and S wall segments in turn and continue recursively
	// as long as the value of r is not within the bounds. If continuing recursively returns an impossible configuration, we
	// backtrack and try a wall segment of the next length instead. Note that we should prefer to use the long segments first since
	// they can be replaced by gates.

	for (let candSegment of candidateSegments)
	{
		segments.push(candSegment);

		let newDistSoFar = distSoFar + candSegment.len;
		let r = d - newDistSoFar;

		let rLowerBound = (1 - 2 * maxOverlap) * segments.length * t;
		let rUpperBound = (1 - 2 * minOverlap) * segments.length * t;

		if (r < rLowerBound)
		{
			// we've allocated too much wall length, pop the last segment and try the next
			//warn("Distance so far exceeds target, trying next level");
			segments.pop();
			continue;
		}
		else if (r > rUpperBound)
		{
			let recursiveResult = GetWallSegmentsRec(d, candidateSegments, minOverlap, maxOverlap, t, newDistSoFar, segments);
			if (!recursiveResult)
			{
				// recursive search with this piece yielded no results, pop it and try the next one
				segments.pop();
				continue;
			}

			return recursiveResult;
		}

		return { "segments": segments, "r": r };
	}

	return false;
}

Engine.RegisterGlobal("GetWallPlacement", GetWallPlacement);
