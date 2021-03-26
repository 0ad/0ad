function PositionHelper() {}

/**
 * @param {number} firstEntity - The entityID of an entity.
 * @param {number} secondEntity - The entityID of an entity.
 *
 * @return {number} - The horizontal distance between the two given entities. Returns
 *			infinity when the distance cannot be calculated.
 */
PositionHelper.prototype.DistanceBetweenEntities = function(firstEntity, secondEntity)
{
	let cmpFirstPosition = Engine.QueryInterface(firstEntity, IID_Position);
	if (!cmpFirstPosition || !cmpFirstPosition.IsInWorld())
		return Infinity;

	let cmpSecondPosition = Engine.QueryInterface(secondEntity, IID_Position);
	if (!cmpSecondPosition || !cmpSecondPosition.IsInWorld())
		return Infinity;

	return cmpFirstPosition.GetPosition2D().distanceTo(cmpSecondPosition.GetPosition2D());
};

/**
 * @param {Vector2D} origin - The point to check around.
 * @param {number}   radius - The radius around the point to check.
 * @param {number[]} players - The players of which we need to check entities.
 * @param {number}   iid - Interface IID that returned entities must implement. Defaults to none.
 *
 * @return {number[]} The id's of the entities in range of the given point.
 */
PositionHelper.prototype.EntitiesNearPoint = function(origin, radius, players, iid = 0)
{
	if (!origin || !radius || !players || !players.length)
		return [];

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return cmpRangeManager.ExecuteQueryAroundPos(origin, 0, radius, players, iid, true);
};

/**
 * Gives the position of the given entity, taking the lateness into account.
 * Note that vertical movement is ignored.
 *
 * @param {number} ent - Entity id of the entity we are finding the location for.
 * @param {number} lateness - The time passed since the expected time to fire the function.
 *
 * @return {Vector3D} The interpolated location of the entity.
 */
PositionHelper.prototype.InterpolatedLocation = function(ent, lateness)
{
	let cmpTargetPosition = Engine.QueryInterface(ent, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld()) // TODO: handle dead target properly
		return undefined;
	let curPos = cmpTargetPosition.GetPosition();
	let prevPos = cmpTargetPosition.GetPreviousPosition();
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let turnLength = cmpTimer.GetLatestTurnLength();
	return new Vector3D(
	    (curPos.x * (turnLength - lateness) + prevPos.x * lateness) / turnLength,
	    0,
	    (curPos.z * (turnLength - lateness) + prevPos.z * lateness) / turnLength
	);
};

/**
 * Test if a point is inside an entity's footprint.
 * Note that edges may be not included for square entities due to rounding.
 *
 * @param {number} ent - Id of the entity we are checking with.
 * @param {Vector3D} point - The point we are checking with.
 * @param {number} lateness - The time passed since the expected time to fire the function.
 *
 * @return {boolean} True if the point is inside of the entity's footprint.
 */
PositionHelper.prototype.TestCollision = function(ent, point, lateness)
{
	let targetPosition = this.InterpolatedLocation(ent, lateness);
	if (!targetPosition)
		return false;

	let cmpFootprint = Engine.QueryInterface(ent, IID_Footprint);
	if (!cmpFootprint)
		return false;

	let targetShape = cmpFootprint.GetShape();
	if (!targetShape)
		return false;

	if (targetShape.type == "circle")
		return targetPosition.horizDistanceToSquared(point) < targetShape.radius * targetShape.radius;

	if (targetShape.type == "square")
	{
		let angle = Engine.QueryInterface(ent, IID_Position).GetRotation().y;
		let distance = Vector2D.from3D(Vector3D.sub(point, targetPosition)).rotate(-angle);
		return Math.abs(distance.x) < targetShape.width / 2 && Math.abs(distance.y) < targetShape.depth / 2;
	}

	warn("TestCollision called with an invalid footprint shape: " + targetShape.type + ".");
	return false;
};

/**
 * Get the predicted time of collision between a projectile (or a chaser)
 * and its target, assuming they both move in straight line at a constant speed.
 * Vertical component of movement is ignored.
 *
 * @param {Vector3D} firstPosition - The 3D position of the projectile (or chaser).
 * @param {number} selfSpeed - The horizontal speed of the projectile (or chaser).
 * @param {Vector3D} targetPosition - The 3D position of the target.
 * @param {Vector3D} targetVelocity - The 3D velocity vector of the target.
 *
 * @return {number|boolean} - The time to collision or false if the collision will not happen.
 */
PositionHelper.prototype.PredictTimeToTarget = function(firstPosition, selfSpeed, targetPosition, targetVelocity)
{
	let relativePosition = new Vector3D.sub(targetPosition, firstPosition);
	let a = targetVelocity.x * targetVelocity.x + targetVelocity.z * targetVelocity.z - selfSpeed * selfSpeed;
	let b = relativePosition.x * targetVelocity.x + relativePosition.z * targetVelocity.z;
	let c = relativePosition.x * relativePosition.x + relativePosition.z * relativePosition.z;

	// The predicted time to reach the target is the smallest non negative solution
	// (when it exists) of the equation a t^2 + 2 b t + c = 0.
	// Using c>=0, we can straightly compute the right solution.

	if (c == 0)
		return 0;

	let disc = b * b - a * c;
	if (a < 0 || b < 0 && disc >= 0)
		return c / (Math.sqrt(disc) - b);

	return false;
};

/**
 * @param {number} target - EntityID to find the spawn position for.
 * @param {number} entity - EntityID to find the spawn position for.
 * @param {boolean} forced - Optionally whether the spawning is forced.
 * @return {Vector3D} - An appropriate spawning position.
 */
PositionHelper.prototype.GetSpawnPosition = function(target, entity, forced)
{
	let cmpFootprint = Engine.QueryInterface(target, IID_Footprint);
	let cmpHealth = Engine.QueryInterface(target, IID_Health);
	let cmpIdentity = Engine.QueryInterface(target, IID_Identity);

	if (!cmpFootprint)
		return null;

	// If the spawner is a sinking ship, restrict the location to the intersection of both passabilities.
	// TODO: should use passability classes to be more generic.
	let pos;
	if ((!cmpHealth || cmpHealth.GetHitpoints() == 0) && cmpIdentity && cmpIdentity.HasClass("Ship"))
		pos = cmpFootprint.PickSpawnPointBothPass(entity);
	else
		pos = cmpFootprint.PickSpawnPoint(entity);

	if (pos.y < 0)
	{
		if (!forced)
			return null;

		// If ejection is forced, we need to continue, so use center of the entity.
		let cmpPosition = Engine.QueryInterface(target, IID_Position);
		pos = cmpPosition.GetPosition();
	}
	return pos;
};

Engine.RegisterGlobal("PositionHelper", new PositionHelper());
