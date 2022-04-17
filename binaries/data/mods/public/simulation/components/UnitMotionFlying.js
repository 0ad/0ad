// (A serious implementation of this might want to use C++ instead of JS
// for performance; this is just for fun.)
const SHORT_FINAL = 2.5;
function UnitMotionFlying() {}

UnitMotionFlying.prototype.Schema =
	"<element name='MaxSpeed'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='TakeoffSpeed'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='StationaryDistance' a:help='Allows the object to be stationary when reaching a target. Value defines the maximum distance at which a target is considered reached.'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='LandingSpeed'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='AccelRate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='SlowingRate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='BrakingRate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='TurnRate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='OvershootTime'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='FlyingHeight'>" +
		"<data type='decimal'/>" +
	"</element>" +
	"<element name='ClimbRate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='DiesInWater'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='PassabilityClass'>" +
		"<text/>" +
	"</element>";

UnitMotionFlying.prototype.Init = function()
{
	this.hasTarget = false;
	this.reachedTarget = false;
	this.targetX = 0;
	this.targetZ = 0;
	this.targetMinRange = 0;
	this.targetMaxRange = 0;
	this.speed = 0;
	this.landing = false;
	this.onGround = true;
	this.pitch = 0;
	this.roll = 0;
	this.waterDeath = false;
	this.passabilityClass = Engine.QueryInterface(SYSTEM_ENTITY, IID_Pathfinder).GetPassabilityClass(this.template.PassabilityClass);
};

UnitMotionFlying.prototype.OnUpdate = function(msg)
{
	let turnLength = msg.turnLength;
	if (!this.hasTarget)
		return;
	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	let pos = cmpPosition.GetPosition();
	let angle = cmpPosition.GetRotation().y;
	let cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	let cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
	let ground = Math.max(cmpTerrain.GetGroundLevel(pos.x, pos.z), cmpWaterManager.GetWaterLevel(pos.x, pos.z));
	let newangle = angle;
	let canTurn = true;
	let distanceToTargetSquared = Math.euclidDistance2DSquared(pos.x, pos.z, this.targetX, this.targetZ);
	if (this.landing)
	{
		if (this.speed > 0 && this.onGround)
		{
			if (pos.y <= cmpWaterManager.GetWaterLevel(pos.x, pos.z) && this.template.DiesInWater == "true")
				this.waterDeath = true;
			this.pitch = 0;
			// Deaccelerate forwards...at a very reduced pace.
			if (this.waterDeath)
				this.speed = Math.max(0, this.speed - turnLength * this.template.BrakingRate * 10);
			else
				this.speed = Math.max(0, this.speed - turnLength * this.template.BrakingRate);
			canTurn = false;
			// Clamp to ground if below it, or descend if above.
			if (pos.y < ground)
				pos.y = ground;
			else if (pos.y > ground)
				pos.y = Math.max(ground, pos.y - turnLength * this.template.ClimbRate);
		}
		else if (this.speed == 0 && this.onGround)
		{
			let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
			if (this.waterDeath && cmpHealth)
				cmpHealth.Kill();
			else
			{
				this.pitch = 0;
				// We've stopped.
				if (cmpGarrisonHolder)
					cmpGarrisonHolder.AllowGarrisoning(true, "UnitMotionFlying");
				canTurn = false;
				this.hasTarget = false;
				this.landing = false;
				// Summon planes back from the edge of the map.
				let terrainSize = cmpTerrain.GetMapSize();
				let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				if (cmpRangeManager.GetLosCircular())
				{
					let mapRadius = terrainSize/2;
					let x = pos.x - mapRadius;
					let z = pos.z - mapRadius;
					let div = (mapRadius - 12) / Math.sqrt(x*x + z*z);
					if (div < 1)
					{
						pos.x = mapRadius + x*div;
						pos.z = mapRadius + z*div;
						newangle += Math.PI;
						distanceToTargetSquared = Math.euclidDistance2DSquared(pos.x, pos.z, this.targetX, this.targetZ);
					}
				}
				else
				{
					pos.x = Math.max(Math.min(pos.x, terrainSize - 12), 12);
					pos.z = Math.max(Math.min(pos.z, terrainSize - 12), 12);
					newangle += Math.PI;
					distanceToTargetSquared = Math.euclidDistance2DSquared(pos.x, pos.z, this.targetX, this.targetZ);
				}
			}
		}
		else
		{
			// Final Approach.
			// We need to slow down to land!
			this.speed = Math.max(this.template.LandingSpeed, this.speed - turnLength * this.template.SlowingRate);
			canTurn = false;
			let targetHeight = ground;
			// Steep, then gradual descent.
			if ((pos.y - targetHeight) / this.template.FlyingHeight > 1 / SHORT_FINAL)
				this.pitch = -Math.PI / 18;
			else
				this.pitch = Math.PI / 18;
			let descentRate = ((pos.y - targetHeight) / this.template.FlyingHeight * this.template.ClimbRate + SHORT_FINAL) * SHORT_FINAL;
			if (pos.y < targetHeight)
				pos.y = Math.max(targetHeight, pos.y + turnLength * descentRate);
			else if (pos.y > targetHeight)
				pos.y = Math.max(targetHeight, pos.y - turnLength * descentRate);
			if (targetHeight == pos.y)
			{
				this.onGround = true;
				if (targetHeight == cmpWaterManager.GetWaterLevel(pos.x, pos.z) && this.template.DiesInWater)
					this.waterDeath = true;
			}
		}
	}
	else
	{
		if (this.template.StationaryDistance && distanceToTargetSquared <= +this.template.StationaryDistance * +this.template.StationaryDistance)
		{
			cmpPosition.SetXZRotation(0, 0);
			this.pitch = 0;
			this.roll = 0;
			this.reachedTarget = true;
			cmpPosition.TurnTo(Math.atan2(this.targetX - pos.x, this.targetZ - pos.z));
			Engine.PostMessage(this.entity, MT_MotionUpdate, { "updateString": "likelySuccess" });
			return;
		}
		// If we haven't reached max speed yet then we're still on the ground;
		// otherwise we're taking off or flying.
		// this.onGround in case of a go-around after landing (but not fully stopped).

		if (this.speed < this.template.TakeoffSpeed && this.onGround)
		{
			if (cmpGarrisonHolder)
				cmpGarrisonHolder.AllowGarrisoning(false, "UnitMotionFlying");
			this.pitch = 0;
			// Accelerate forwards.
			this.speed = Math.min(this.template.MaxSpeed, this.speed + turnLength * this.template.AccelRate);
			canTurn = false;
			// Clamp to ground if below it, or descend if above.
			if (pos.y < ground)
				pos.y = ground;
			else if (pos.y > ground)
				pos.y = Math.max(ground, pos.y - turnLength * this.template.ClimbRate);
		}
		else
		{
			this.onGround = false;
			// Climb/sink to max height above ground.
			this.speed = Math.min(this.template.MaxSpeed, this.speed + turnLength * this.template.AccelRate);
			let targetHeight = ground + (+this.template.FlyingHeight);
			if (Math.abs(pos.y-targetHeight) > this.template.FlyingHeight/5)
			{
				this.pitch = Math.PI / 9;
				canTurn = false;
			}
			else
				this.pitch = 0;
			if (pos.y < targetHeight)
				pos.y = Math.min(targetHeight, pos.y + turnLength * this.template.ClimbRate);
			else if (pos.y > targetHeight)
			{
				pos.y = Math.max(targetHeight, pos.y - turnLength * this.template.ClimbRate);
				this.pitch = -1 * this.pitch;
			}
		}
	}

	// If we're in range of the target then tell people that we've reached it.
	// (TODO: quantisation breaks this)
	if (!this.reachedTarget &&
		this.targetMinRange * this.targetMinRange <= distanceToTargetSquared &&
		distanceToTargetSquared <= this.targetMaxRange * this.targetMaxRange)
	{
		this.reachedTarget = true;
		Engine.PostMessage(this.entity, MT_MotionUpdate, { "updateString": "likelySuccess" });
	}

	// If we're facing away from the target, and are still fairly close to it,
	// then carry on going straight so we overshoot in a straight line.
	let isBehindTarget = ((this.targetX - pos.x) * Math.sin(angle) + (this.targetZ - pos.z) * Math.cos(angle) < 0);
	// Overshoot the target: carry on straight.
	if (isBehindTarget && distanceToTargetSquared < this.template.MaxSpeed * this.template.MaxSpeed * this.template.OvershootTime * this.template.OvershootTime)
		canTurn = false;

	if (canTurn)
	{
		// Turn towards the target.
		let targetAngle = Math.atan2(this.targetX - pos.x, this.targetZ - pos.z);
		let delta = targetAngle - angle;
		// Wrap delta to -pi..pi.
		delta = (delta + Math.PI) % (2*Math.PI);
		if (delta < 0)
			delta += 2 * Math.PI;
		delta -= Math.PI;
		// Clamp to max rate.
		let deltaClamped = Math.min(Math.max(delta, -this.template.TurnRate * turnLength), this.template.TurnRate * turnLength);
		// Calculate new orientation, in a peculiar way in order to make sure the
		// result gets close to targetAngle (rather than being n*2*pi out).
		newangle = targetAngle + deltaClamped - delta;
		if (newangle - angle > Math.PI / 18)
			this.roll = Math.PI / 9;
		else if (newangle - angle < -Math.PI / 18)
			this.roll = -Math.PI / 9;
		else
			this.roll = newangle - angle;
	}
	else
		this.roll = 0;

	pos.x += this.speed * turnLength * Math.sin(angle);
	pos.z += this.speed * turnLength * Math.cos(angle);
	cmpPosition.SetHeightFixed(pos.y);
	cmpPosition.TurnTo(newangle);
	cmpPosition.SetXZRotation(this.pitch, this.roll);
	cmpPosition.MoveTo(pos.x, pos.z);
};

UnitMotionFlying.prototype.MoveToPointRange = function(x, z, minRange, maxRange)
{
	this.hasTarget = true;
	this.landing = false;
	this.reachedTarget = false;
	this.targetX = x;
	this.targetZ = z;
	this.targetMinRange = minRange;
	this.targetMaxRange = maxRange;

	return true;
};

UnitMotionFlying.prototype.MoveToTargetRange = function(target, minRange, maxRange)
{
	let cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return false;

	let targetPos = cmpTargetPosition.GetPosition2D();

	this.hasTarget = true;
	this.reachedTarget = false;
	this.targetX = targetPos.x;
	this.targetZ = targetPos.y;
	this.targetMinRange = minRange;
	this.targetMaxRange = maxRange;

	return true;
};

UnitMotionFlying.prototype.SetMemberOfFormation = function()
{
	// Ignored.
};

UnitMotionFlying.prototype.GetWalkSpeed = function()
{
	return +this.template.MaxSpeed;
};

UnitMotionFlying.prototype.SetSpeedMultiplier = function(multiplier)
{
	// Ignore this, the speed is always the walk speed.
};

UnitMotionFlying.prototype.GetRunMultiplier = function()
{
	return 1;
};

/**
 * Estimate the next position of the unit. Just linearly extrapolate.
 * TODO: Reuse the movement code for a better estimate.
 */
UnitMotionFlying.prototype.EstimateFuturePosition = function(dt)
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return Vector2D();
	let position = cmpPosition.GetPosition2D();

	return Vector2D.add(position, Vector2D.sub(position, cmpPosition.GetPreviousPosition2D()).mult(dt/Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer).GetLatestTurnLength()));
};

UnitMotionFlying.prototype.IsMoveRequested = function()
{
	return this.hasTarget;
};

UnitMotionFlying.prototype.GetCurrentSpeed = function()
{
	return this.speed;
};

UnitMotionFlying.prototype.GetSpeedMultiplier = function()
{
	return this.speed / +this.template.MaxSpeed;
};

UnitMotionFlying.prototype.GetAcceleration = function()
{
	return +this.template.AccelRate;
};

UnitMotionFlying.prototype.SetAcceleration = function()
{
	// Acceleration is set by the template. Ignore.
};

UnitMotionFlying.prototype.GetPassabilityClassName = function()
{
	return this.passabilityClassName ? this.passabilityClassName : this.template.PassabilityClass;
};

UnitMotionFlying.prototype.SetPassabilityClassName = function(passClassName)
{
	this.passabilityClassName = passClassName;
	const cmpPathfinder = Engine.QueryInterface(SYSTEM_ENTITY, IID_Pathfinder);
	if (cmpPathfinder)
		this.passabilityClass = cmpPathfinder.GetPassabilityClass(passClassName);
};

UnitMotionFlying.prototype.GetPassabilityClass = function()
{
	return this.passabilityClass;
};

UnitMotionFlying.prototype.FaceTowardsPoint = function(x, z)
{
	// Ignore this - angle is controlled by the target-seeking code instead.
};

UnitMotionFlying.prototype.SetFacePointAfterMove = function()
{
	// Ignore this - angle is controlled by the target-seeking code instead.
};

UnitMotionFlying.prototype.StopMoving = function()
{
	// Invert.
	if (!this.waterDeath)
		this.landing = !this.landing;

};

UnitMotionFlying.prototype.SetDebugOverlay = function(enabled)
{
};

Engine.RegisterComponentType(IID_UnitMotion, "UnitMotionFlying", UnitMotionFlying);
