Engine.LoadHelperScript("Position.js");
Engine.LoadComponentScript("interfaces/Timer.js");

class testDistanceBetweenEntities
{
	constructor()
	{
		this.firstEntity = 1;
		this.secondEntity = 2;
	}

	testOutOfWorldEntity()
	{
		AddMock(this.firstEntity, IID_Position, {
			"GetPosition2D": () => new Vector3D(1, 0, 0),
			"IsInWorld": () => false
		});
		AddMock(this.secondEntity, IID_Position, {
			"GetPosition2D": () => new Vector3D(2, 0, 0),
			"IsInWorld": () => true
		});

		TS_ASSERT_EQUALS(PositionHelper.DistanceBetweenEntities(this.firstEntity, this.secondEntity), Infinity);

		DeleteMock(this.firstEntity, IID_Position);
		DeleteMock(this.secondEntity, IID_Position);
	}

	testDistanceBetweenEntities(positionOne, positionTwo, expectedDistance)
	{
		AddMock(this.firstEntity, IID_Position, {
			"GetPosition2D": () => positionOne,
			"IsInWorld": () => true
		});
		AddMock(this.secondEntity, IID_Position, {
			"GetPosition2D": () => positionTwo,
			"IsInWorld": () => true
		});

		TS_ASSERT_EQUALS(PositionHelper.DistanceBetweenEntities(this.firstEntity, this.secondEntity), expectedDistance);

		DeleteMock(this.firstEntity, IID_Position);
		DeleteMock(this.secondEntity, IID_Position);
	}

	test()
	{
		this.testOutOfWorldEntity();

		this.testDistanceBetweenEntities(new Vector3D(1, 0, 0), new Vector3D(0, 0, 0), 1);
		this.testDistanceBetweenEntities(new Vector3D(0, 0, 0), new Vector3D(0, 2, 0), 2);
		this.testDistanceBetweenEntities(new Vector3D(1, 2, 5), new Vector3D(4, 2, 5), 3);
		this.testDistanceBetweenEntities(new Vector3D(7, 7, 7), new Vector3D(7, 7, 7), 0);
	}
}

class testInterpolatedLocation
{
	constructor()
	{
		this.entity = 1;
		this.turnLength = 200;
		AddMock(SYSTEM_ENTITY, IID_Timer, {
			"GetLatestTurnLength": () => this.turnLength
		});
	}

	testOutOfWorldEntity()
	{
		AddMock(this.entity, IID_Position, {
			"IsInWorld": () => false
		});
		TS_ASSERT_EQUALS(PositionHelper.InterpolatedLocation(this.entity, 0), undefined);

		DeleteMock(this.entity, IID_Position);
	}

	testInterpolatedLocation(previousPosition, currentPosition, expectedPosition, lateness)
	{
		AddMock(this.entity, IID_Position, {
			"GetPreviousPosition": () => previousPosition,
			"GetPosition": () => currentPosition,
			"IsInWorld": () => true
		});
		TS_ASSERT_UNEVAL_EQUALS(PositionHelper.InterpolatedLocation(this.entity, lateness), expectedPosition);

		DeleteMock(this.entity, IID_Position);
	}

	test()
	{
		this.testOutOfWorldEntity();

		this.testInterpolatedLocation(new Vector3D(1, 0, 1), new Vector3D(2, 0, 2), new Vector3D(2, 0, 2), 0);
		this.testInterpolatedLocation(new Vector3D(1, 0, 1), new Vector3D(2, 0, 2), new Vector3D(1, 0, 1), this.turnLength);

		this.testInterpolatedLocation(new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), this.turnLength / 2);
		this.testInterpolatedLocation(new Vector3D(0, 0, 0), new Vector3D(0, 0, 2), new Vector3D(0, 0, 1), this.turnLength / 2);
		this.testInterpolatedLocation(new Vector3D(0, 0, 1), new Vector3D(0, 0, 2), new Vector3D(0, 0, 1.5), this.turnLength / 2);
		this.testInterpolatedLocation(new Vector3D(0, 0, 1), new Vector3D(0, 0, 5), new Vector3D(0, 0, 4), this.turnLength / 4);
		this.testInterpolatedLocation(new Vector3D(0, 0, -1), new Vector3D(0, 0, 3), new Vector3D(0, 0, 1), this.turnLength / 2);
		this.testInterpolatedLocation(new Vector3D(0, 0, -1), new Vector3D(0, 0, -3), new Vector3D(0, 0, -2), this.turnLength / 2);

		// Y is ignored.
		this.testInterpolatedLocation(new Vector3D(1, 1, 1), new Vector3D(3, 3, 3), new Vector3D(2.5, 0, 2.5), this.turnLength / 4);
		this.testInterpolatedLocation(new Vector3D(0, 1, 0), new Vector3D(0, 3, 0), new Vector3D(0, 0, 0), this.turnLength / 2);
	}
}

class testTestCollision
{
	constructor()
	{
		this.entity = 1;
		AddMock(SYSTEM_ENTITY, IID_Timer, {
			"GetLatestTurnLength": () => 200
		});
	}

	testOutOfWorldEntity()
	{
		AddMock(this.entity, IID_Position, {
			"IsInWorld": () => false
		});
		TS_ASSERT(!PositionHelper.TestCollision(this.entity, new Vector3D(0, 0, 0), 0));

		DeleteMock(this.entity, IID_Position);
	}

	testFootprintlessEntity()
	{
		AddMock(this.entity, IID_Position, {
			"GetPreviousPosition": () => new Vector3D(0, 0, 0),
			"GetPosition": () => new Vector3D(0, 0, 0),
			"IsInWorld": () => true
		});
		TS_ASSERT(!PositionHelper.TestCollision(this.entity, new Vector3D(0, 0, 0), 0));

		AddMock(this.entity, IID_Footprint, {
			"GetShape": () => {}
		});
		TS_ASSERT(!PositionHelper.TestCollision(this.entity, new Vector3D(0, 0, 0), 0));

		DeleteMock(this.entity, IID_Position);
		DeleteMock(this.entity, IID_Footprint);
	}

	testTestCollision(footprint, collisionPoint, entityPosition, expectedResult)
	{
		PositionHelper.InterpolatedLocation = (ent, lateness) => entityPosition;
		AddMock(this.entity, IID_Footprint, {
			"GetShape": () => footprint
		});

		TS_ASSERT_EQUALS(PositionHelper.TestCollision(this.entity, collisionPoint, 0), expectedResult);

		DeleteMock(this.entity, IID_Footprint);
	}

	testCircularFootprints()
	{
		// Interesting edge-case.
		this.testTestCollision({ "type": "circle", "radius": 0 }, new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), false);

		this.testTestCollision({ "type": "circle", "radius": 2 }, new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), true);
		this.testTestCollision({ "type": "circle", "radius": 2 }, new Vector3D(0, 0, 0), new Vector3D(0, 0, 1), true);
		this.testTestCollision({ "type": "circle", "radius": 2 }, new Vector3D(0, 0, 0), new Vector3D(2, 0, 2), false);
		this.testTestCollision({ "type": "circle", "radius": 3 }, new Vector3D(-1, 0, -1), new Vector3D(1, 0, 1), true);
		this.testTestCollision({ "type": "circle", "radius": 3 }, new Vector3D(-1, 0, 1), new Vector3D(1, 0, -1), true);

		// Y is ignored.
		this.testTestCollision({ "type": "circle", "radius": 2 }, new Vector3D(0, 0, 0), new Vector3D(1, 5, 1), true);
		this.testTestCollision({ "type": "circle", "radius": 1 }, new Vector3D(0, -100, 0), new Vector3D(0, 100, 0), true);
	}

	/**
	 * Edges may be not colliding due to rounding issues.
	 */
	testSquareFootprints()
	{
		let square = { "type": "square", "width": 2, "depth": 2 };
		AddMock(this.entity, IID_Position, {
			"GetRotation": () => new Vector3D(0, 0, 0)
		});

		// Interesting edge-case.
		this.testTestCollision({ "type": "square", "width": 0, "depth": 0 }, new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), false);

		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(0, 0, 0), true);
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(0.999, 0, 0), true);
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(0.999, 0, 0.999), true);
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(2, 0, 2), false);
		this.testTestCollision(square, new Vector3D(-1, 0, -1), new Vector3D(-0.001, 0, -0.001), true);
		this.testTestCollision({ "type": "square", "width": 4, "depth": 4 }, new Vector3D(-1, 0, 1), new Vector3D(0.999, 0, -0.999), true);

		// Y is ignored.
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(0.5, 10, 0.5), true);
		this.testTestCollision(square, new Vector3D(-1, 50, 0), new Vector3D(-0.5, 10, 0), true);

		// Test rotated.
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(Math.sqrt(2), 0, 0), false);
		AddMock(this.entity, IID_Position, {
			"GetRotation": () => new Vector3D(0, Math.PI / 4, 0)
		});
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(Math.sqrt(2), 0, 0), true);
		this.testTestCollision(square, new Vector3D(0, 0, 0), new Vector3D(0.999, 0, 0.999), false);

		DeleteMock(this.entity, IID_Position);
	}

	/**
	 * Edges may be not colliding due to rounding issues.
	 */
	testRectangularFootprints()
	{
		let rectangle = { "type": "square", "width": 2, "depth": 4 };
		AddMock(this.entity, IID_Position, {
			"GetRotation": () => new Vector3D(0, 0, 0)
		});

		this.testTestCollision(rectangle, new Vector3D(0, 0, 0), new Vector3D(1.999, 0, 0), false);
		this.testTestCollision(rectangle, new Vector3D(0, 0, 0), new Vector3D(0, 0, 1.999), true);

		AddMock(this.entity, IID_Position, {
			"GetRotation": () => new Vector3D(0, Math.PI / 2, 0)
		});

		this.testTestCollision(rectangle, new Vector3D(0, 0, 0), new Vector3D(1.999, 0, 0), true);
		this.testTestCollision(rectangle, new Vector3D(0, 0, 0), new Vector3D(0, 0, 1.999), false);

		DeleteMock(this.entity, IID_Position);
	}

	test()
	{
		this.testOutOfWorldEntity();
		this.testFootprintlessEntity();
		this.testCircularFootprints();
		this.testSquareFootprints();
		this.testRectangularFootprints();
	}
}

class testPredictTimeToTarget
{
	constructor()
	{
		this.uncertainty = 0.0001;
	}

	testPredictTimeToTarget(selfPosition, selfSpeed, targetPosition, targetVelocity)
	{
		const timeToTarget = PositionHelper.PredictTimeToTarget(selfPosition, selfSpeed, targetPosition, targetVelocity);
		if (timeToTarget === false)
			return;
		// Position of the target after that time.
		const targetPos = Vector3D.mult(targetVelocity, timeToTarget).add(targetPosition);
		// Time that the projectile need to reach it.
		const time = targetPos.horizDistanceTo(selfPosition) / selfSpeed;
		TS_ASSERT(Math.abs(timeToTarget - time) < this.uncertainty);
	}

	/**
	 * What is at least tested:
	 * - Moving away from us in a straight line.
	 * - Moving towards us in a straight line.
	 * - Moving from us in right angle.
	 * - Moving in an angle away from us.
	 * - Moving in an angle towards us.
	 */
	test()
	{
		// Not able to move at all.
		this.testPredictTimeToTarget(new Vector3D(0, 0, 0), 0, new Vector3D(1, 0, 0), new Vector3D(0, 0, 0));

		const selfPositions = [
			new Vector3D(0, 0, 0),
			new Vector3D(1, 0, 0),
			new Vector3D(4, 0, 0),
			new Vector3D(-1, 0, 0),
			new Vector3D(-4, 2, 0),
			new Vector3D(0, 0, 1),
			new Vector3D(0, 0, 4),
			new Vector3D(1, 0, 1),
			new Vector3D(2, 0, 2),
			new Vector3D(-1, 3, 1),
			new Vector3D(-2, 0, 2),
			new Vector3D(4, 0, 2),
			new Vector3D(-4, 1, 2)
		];
		const selfSpeeds = [1, 2, 3, 4, 5, 6];
		const targetPositions = [
			new Vector3D(0, 0, 0),
			new Vector3D(1, 0, 0),
			new Vector3D(3, 0, 0),
			new Vector3D(-1, 0, 0),
			new Vector3D(-3, 1, 0),
			new Vector3D(0, 0, 1),
			new Vector3D(0, 0, 3),
			new Vector3D(1, 0, 1),
			new Vector3D(5, 0, 5),
			new Vector3D(-1, 0, 1),
			new Vector3D(-3, 0, 2),
			new Vector3D(4, 0, 2),
			new Vector3D(-6, 8, 2)
		];
		const targetVelocities = [
			new Vector3D(0, 0, 0),
			new Vector3D(1, 0, 0),
			new Vector3D(4, 0, 0),
			new Vector3D(16, 0, 0),
			new Vector3D(-1, 0, 0),
			new Vector3D(-4, 0, 0),
			new Vector3D(-16, 0, 0),
			new Vector3D(0, 2, 1),
			new Vector3D(0, 0, 4),
			new Vector3D(0, 0, 16),
			new Vector3D(1, 0, 1),
			new Vector3D(2, 1, 2),
			new Vector3D(8, 0, 8),
			new Vector3D(-1, 0, 1),
			new Vector3D(-2, 0, 2),
			new Vector3D(-8, 0, 8),
			new Vector3D(4, 0, 2),
			new Vector3D(-4, 0, 2)
		];

		for (const selfPosition of selfPositions)
			for (const selfSpeed of selfSpeeds)
				for (const targetPosition of targetPositions)
					for (const targetVelocity of targetVelocities)
						this.testPredictTimeToTarget(selfPosition, selfSpeed, targetPosition, targetVelocity);
	}
}

new testDistanceBetweenEntities().test();
new testInterpolatedLocation().test();
new testTestCollision().test();
new testPredictTimeToTarget().test();
