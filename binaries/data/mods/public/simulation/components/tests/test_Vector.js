var brokenVector = {
	"lengthSquared": () => "incompatible vector"
};

// Test Vector2D add, mult, distance
{
	let v1 = new Vector2D();
	TS_ASSERT_EQUALS(v1.x, 0);
	TS_ASSERT_EQUALS(v1.y, 0);

	let v2 = new Vector2D(3, 4);
	TS_ASSERT_EQUALS(v1.distanceTo(v2), 5);

	v2.mult(3);
	TS_ASSERT_EQUALS(v2.x, 9);
	TS_ASSERT_EQUALS(v2.y, 12);

	v2.add(new Vector2D(1, 2));
	TS_ASSERT_EQUALS(v2.x, 10);
	TS_ASSERT_EQUALS(v2.y, 14);
}

// Test Vector2D normalization
{
	let v3 = new Vector2D(0, 5).normalize();
	TS_ASSERT_EQUALS(v3.x, 0);
	TS_ASSERT_EQUALS(v3.y, 1);

	v3.set(-8, 0).normalize();
	TS_ASSERT_EQUALS(v3.x, -1);
	TS_ASSERT_EQUALS(v3.y, 0);
}

// Test Vector2D rotation
{
	let v4 = new Vector2D(2, -5).rotate(4 * Math.PI);
	TS_ASSERT_EQUALS(v4.x, 2);
	TS_ASSERT_EQUALS(v4.y, -5);

	v4.rotate(Math.PI);
	TS_ASSERT_EQUALS(v4.x, -2);
	TS_ASSERT_EQUALS(v4.y, 5);

	// Result of rotating (1, 0)
	let unitCircle = [
		{
			"angle": Math.PI / 2,
			"x": 0,
			"y": 1
		},
		{
			"angle": Math.PI / 3,
			"x": 1/2,
			"y": Math.sqrt(3) / 2
		},
		{
			"angle": Math.PI / 4,
			"x": Math.sqrt(2) / 2,
			"y": Math.sqrt(2) / 2
		},
		{
			"angle": Math.PI / 6,
			"x": Math.sqrt(3) / 2,
			"y": 1/2
		}
	];

	let epsilon = 0.00000001;
	for (let expectedVector of unitCircle)
	{
		let computedVector = new Vector2D(1, 0).rotate(-expectedVector.angle);
		TS_ASSERT_EQUALS_APPROX(computedVector.x, expectedVector.x, epsilon);
		TS_ASSERT_EQUALS_APPROX(computedVector.y, expectedVector.y, epsilon);
	}
}

// Test Vector2D rotation around a center
{
	let epsilon = 0.00000001;
	let v1 = new Vector2D(-4, 8).rotateAround(Math.PI / 3, new Vector2D(-1, -3));
	TS_ASSERT_EQUALS_APPROX(v1.x, 7.02627944, epsilon);
	TS_ASSERT_EQUALS_APPROX(v1.y, 5.09807617, epsilon);
}

// Test Vector2D dot product
{
	TS_ASSERT_EQUALS(new Vector2D(2, 3).dot(new Vector2D(4, 5)), 23);
}

// Test Vector2D cross product
{
	TS_ASSERT_EQUALS(new Vector2D(3, 5).cross(new Vector2D(-4, -1/3)), 19);
}

// Test Vector2D length and compareLength
{
	TS_ASSERT_EQUALS(new Vector2D(20, 21).length(), 29);

	let v5 = new Vector2D(10, 20);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(5, 8)), 1);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(500, 800)), -1);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(10, 20)), 0);
	TS_ASSERT(isNaN(v5.compareLength(brokenVector)));
}

// Test Vector2D rotation furthermore
{
	let epsilon = 0.00000001;
	let v5 = new Vector2D(10, 20);
	let v6 = v5.clone();
	TS_ASSERT_EQUALS(v5.x, v6.x);
	TS_ASSERT_EQUALS(v5.y, v6.y);
	TS_ASSERT(Math.abs(v5.dot(v6.rotate(Math.PI / 2))) < epsilon);
}

// Test Vector2D perpendicular
{
	let v7 = new Vector2D(4, 5).perpendicular();
	TS_ASSERT_EQUALS(v7.x, -5);
	TS_ASSERT_EQUALS(v7.y, 4);

	let v8 = new Vector2D(0, 0).perpendicular();
	TS_ASSERT_EQUALS(v8.x, 0);
	TS_ASSERT_EQUALS(v8.y, 0);
}

// Test Vector2D angleTo
{
	let v1 = new Vector2D(1, 1);
	let v2 = new Vector2D(1, 3);
	let v3 = new Vector2D(3, 1);
	TS_ASSERT_EQUALS(v1.angleTo(v2), 0);
	TS_ASSERT_EQUALS(v1.angleTo(v3), Math.PI / 2);
	TS_ASSERT_EQUALS(v3.angleTo(v2), -Math.PI / 4);
}

// Test Vector2D list functions
{
	let list = [
		new Vector2D(),
		new Vector2D(-1, 5),
		new Vector2D(89, -123),
		new Vector2D(55, 66),
	];

	let sum = Vector2D.sum(list);
	TS_ASSERT_EQUALS(sum.x, 143);
	TS_ASSERT_EQUALS(sum.y, -52);

	let avg = Vector2D.average(list);
	TS_ASSERT_EQUALS(avg.x, 35.75);
	TS_ASSERT_EQUALS(avg.y, -13);
}

// Test Vector2D round
{
	let v1 = new Vector2D(-4.5, 8.2).round();
	TS_ASSERT_EQUALS(v1.x, -4);
	TS_ASSERT_EQUALS(v1.y, 8);

	let v2 = new Vector2D(NaN, NaN).round();
	TS_ASSERT(isNaN(v2.x));
	TS_ASSERT(isNaN(v2.y));

	let v3 = new Vector2D().round();
	TS_ASSERT_EQUALS(v3.x, 0);
	TS_ASSERT_EQUALS(v3.y, 0);
}

// Vector3D tests

// Test Vector3D distance and compareLength
{
	let v1 = new Vector3D(2, 5, 14);
	TS_ASSERT_EQUALS(v1.distanceTo(new Vector3D()), 15);
	TS_ASSERT(isNaN(v1.compareLength(brokenVector)));
}

// Test Vector3D mult
{
	let v2 = new Vector3D(2, 5, 14).mult(3);
	TS_ASSERT_EQUALS(v2.x, 6);
	TS_ASSERT_EQUALS(v2.y, 15);
	TS_ASSERT_EQUALS(v2.z, 42);
}

// Test Vector3D dot product
{
	TS_ASSERT_EQUALS(new Vector3D(1, 2, 3).dot(new Vector3D(4, 5, 6)), 32);
}

// Test Vector3D clone
{
	let v3 = new Vector3D(9, 10, 11);
	let v4 = v3.clone();
	TS_ASSERT_EQUALS(v3.x, v4.x);
	TS_ASSERT_EQUALS(v3.y, v4.y);
	TS_ASSERT_EQUALS(v3.z, v4.z);
}

// Test Vector3D cross product
{
	let v5 = new Vector3D(1, 2, 3).cross(new Vector3D(4, 5, 6));
	TS_ASSERT_EQUALS(v5.x, -3);
	TS_ASSERT_EQUALS(v5.y, 6);
	TS_ASSERT_EQUALS(v5.z, -3);
}

// Test Vector3D horizAngleTo
{
	let v1 = new Vector3D(1, 1, 1);
	let v2 = new Vector3D(1, 2, 3);
	let v3 = new Vector3D(3, 10, 1);
	TS_ASSERT_EQUALS(v1.horizAngleTo(v2), 0);
	TS_ASSERT_EQUALS(v1.horizAngleTo(v3), Math.PI / 2);
	TS_ASSERT_EQUALS(v3.horizAngleTo(v2), -Math.PI / 4);
}

// Test Vector3D round
{
	let v1 = new Vector3D(-1.1, 2.2, 3.3).round();
	TS_ASSERT_EQUALS(v1.x, -1);
	TS_ASSERT_EQUALS(v1.y, 2);
	TS_ASSERT_EQUALS(v1.z, 3);

	let v2 = new Vector3D(NaN, NaN, NaN).round();
	TS_ASSERT(isNaN(v2.x));
	TS_ASSERT(isNaN(v2.y));
	TS_ASSERT(isNaN(v2.z));

	let v3 = new Vector3D().round();
	TS_ASSERT_EQUALS(v3.x, 0);
	TS_ASSERT_EQUALS(v3.y, 0);
	TS_ASSERT_EQUALS(v3.z, 0);

	let v4 = new Vector3D(71.8, 73.4, 73.89).round();
	TS_ASSERT_EQUALS(v4.x, 72);
	TS_ASSERT_EQUALS(v4.y, 73);
	TS_ASSERT_EQUALS(v4.z, 74);
}
