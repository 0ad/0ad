var brokenVector = {
	"lengthSquared": () => "incompatible vector"
};

// Test Vector2D
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

	let v3 = new Vector2D(0, 5).normalize();
	TS_ASSERT_EQUALS(v3.x, 0);
	TS_ASSERT_EQUALS(v3.y, 1);

	v3.set(-8, 0).normalize();
	TS_ASSERT_EQUALS(v3.x, -1);
	TS_ASSERT_EQUALS(v3.y, 0);

	let v4 = new Vector2D(2, -5).rotate(4 * Math.PI);
	TS_ASSERT_EQUALS(v4.x, 2);
	TS_ASSERT_EQUALS(v4.y, -5);

	v4.rotate(Math.PI);
	TS_ASSERT_EQUALS(v4.x, -2);
	TS_ASSERT_EQUALS(v4.y, 5);

	TS_ASSERT_EQUALS(new Vector2D(2, 3).dot(new Vector2D(4, 5)), 23);
	TS_ASSERT_EQUALS(new Vector2D(3, 5).cross(new Vector2D(-4, -1/3)), 19);
	TS_ASSERT_EQUALS(new Vector2D(20, 21).length(), 29);

	let v5 = new Vector2D(10, 20);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(5, 8)), 1);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(500, 800)), -1);
	TS_ASSERT_EQUALS(v5.compareLength(new Vector2D(10, 20)), 0);
	TS_ASSERT(isNaN(v5.compareLength(brokenVector)));
}

// Test Vector3D
{
	let v1 = new Vector3D(2, 5, 14);
	TS_ASSERT_EQUALS(v1.distanceTo(new Vector3D()), 15);
	TS_ASSERT(isNaN(v1.compareLength(brokenVector)));

	let v2 = v1.mult(3);
	TS_ASSERT_EQUALS(v2.x, 6);
	TS_ASSERT_EQUALS(v2.y, 15);
	TS_ASSERT_EQUALS(v2.z, 42);

	TS_ASSERT_EQUALS(new Vector3D(1, 2, 3).dot(new Vector3D(4, 5, 6)), 32);

}