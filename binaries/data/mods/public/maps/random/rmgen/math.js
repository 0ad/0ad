function diskArea(radius)
{
	return Math.PI * Math.square(radius);
}

/**
 * Returns the angle of the vector between point 1 and point 2.
 * The angle is counterclockwise from the positive x axis.
 */
function getAngle(x1, z1, x2, z2)
{
	return Math.atan2(z2 - z1, x2 - x1);
}

function rotateCoordinates(x, z, angle, centerX = 0.5, centerZ = 0.5)
{
	let sin = Math.sin(angle);
	let cos = Math.cos(angle);

	return [
		cos * (x - centerX) - sin * (z - centerZ) + centerX,
		sin * (x - centerX) + cos * (z - centerZ) + centerZ
	];
}
/**
 * Returns the distance of a point from a line.
 */
function distanceOfPointFromLine(line_x1, line_y1, line_x2, line_y2, point_x, point_y)
{
	let width_x = line_x1 - line_x2;
	if (!width_x)
		return Math.abs(point_x - line_x1);

	let width_y = line_y1 - line_y2;
	if (!width_y)
		return Math.abs(point_y - line_y1);

	let inclination = width_y / width_x;
	let intercept = line_y1 - inclination * line_x1;

	return Math.abs((point_y - point_x * inclination - intercept) / Math.sqrt(1 + Math.square(inclination)));
}

/**
 * Determines whether two lines with the given width intersect.
 */
function checkIfIntersect(line1_x1, line1_y1, line1_x2, line1_y2, line2_x1, line2_y1, line2_x2, line2_y2, width)
{
	if (line1_x1 == line1_x2)
	{
		if (line2_x1 - line1_x1 < width || line2_x2 - line1_x2 < width)
			return true;
	}
	else
	{
		let m = (line1_y1 - line1_y2) / (line1_x1 - line1_x2);
		let b = line1_y1 - m * line1_x1;
		let m2 = Math.sqrt(1 + Math.square(m));

		if (Math.abs((line2_y1 - line2_x1 * m - b) / m2) < width || Math.abs((line2_y2 - line2_x2 * m - b) / m2) < width)
			return true;

		if (line2_x1 == line2_x2)
		{
			if (line1_x1 - line2_x1 < width || line1_x2 - line2_x2 < width)
				return true;
		}
		else
		{
			let m = (line2_y1 - line2_y2) / (line2_x1 - line2_x2);
			let b = line2_y1 - m * line2_x1;
			let m2 = Math.sqrt(1 + Math.square(m));
			if (Math.abs((line1_y1 - line1_x1 * m - b) / m2) < width || Math.abs((line1_y2 - line1_x2 * m - b) / m2) < width)
				return true;
		}
	}

	let s = (line1_x1 - line1_x2) * (line2_y1 - line1_y1) - (line1_y1 - line1_y2) * (line2_x1 - line1_x1);
	let p = (line1_x1 - line1_x2) * (line2_y2 - line1_y1) - (line1_y1 - line1_y2) * (line2_x2 - line1_x1);

	if (s * p <= 0)
	{
		s = (line2_x1 - line2_x2) * (line1_y1 - line2_y1) - (line2_y1 - line2_y2) * (line1_x1 - line2_x1);
		p = (line2_x1 - line2_x2) * (line1_y2 - line2_y1) - (line2_y1 - line2_y2) * (line1_x2 - line2_x1);

		if (s * p <= 0)
			return true;
	}

	return false;
}
