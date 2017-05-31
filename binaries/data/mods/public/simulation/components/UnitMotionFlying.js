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
	var turnLength = msg.turnLength;
	if (!this.hasTarget)
		return;
	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var pos = cmpPosition.GetPosition();
	var angle = cmpPosition.GetRotation().y;
	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
	var cmpWaterManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_WaterManager);
	var ground = Math.max(cmpTerrain.GetGroundLevel(pos.x, pos.z), cmpWaterManager.GetWaterLevel(pos.x, pos.z));
	var newangle = angle;
	var canTurn = true;
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
			// Clamp to ground if below it, or descend if above
			if (pos.y < ground)
				pos.y = ground;
			else if (pos.y > ground)
				pos.y = Math.max(ground, pos.y - turnLength * this.template.ClimbRate);
		}
		else if (this.speed == 0 && this.onGround)
		{
			if (this.waterDeath && cmpHealth)
				cmpHealth.Kill();
			else
			{
				this.pitch = 0;
				// We've stopped.
				if (cmpGarrisonHolder)
					cmpGarrisonHolder.AllowGarrisoning(true,"UnitMotionFlying");
				canTurn = false;
				this.hasTarget = false;
				this.landing = false;
				// summon planes back from the edge of the map
				var terrainSize = cmpTerrain.GetMapSize();
				var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
				if (cmpRangeManager.GetLosCircular())
				{
					var mapRadius = terrainSize/2;
					var x = pos.x - mapRadius;
					var z = pos.z - mapRadius;
					var div = (mapRadius - 12) / Math.sqrt(x*x + z*z);
					if (div < 1)
					{
						pos.x = mapRadius + x*div;
						pos.z = mapRadius + z*div;
						newangle += Math.PI;
					}
				}
				else
				{
					pos.x = Math.max(Math.min(pos.x, terrainSize - 12), 12);
					pos.z = Math.max(Math.min(pos.z, terrainSize - 12), 12);
					newangle += Math.PI;
				}
			}
		}
		else
		{
			// Final Approach
			// We need to slow down to land!
			this.speed = Math.max(this.template.LandingSpeed, this.speed - turnLength * this.template.SlowingRate);
			canTurn = false;
			var targetHeight = ground;
			// Steep, then gradual descent.
			if ((pos.y - targetHeight) / this.template.FlyingHeight > 1 / SHORT_FINAL)
				this.pitch = - Math.PI / 18;
			else
				this.pitch = Math.PI / 18;
			var descentRate = ((pos.y - targetHeight) / this.template.FlyingHeight * this.template.ClimbRate + SHORT_FINAL) * SHORT_FINAL;
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
		// If we haven't reached max speed yet then we're still on the ground;
		// otherwise we're taking off or flying
		// this.onGround in case of a go-around after landing (but not fully stopped)

		if (this.speed < this.template.TakeoffSpeed && this.onGround)
		{
			if (cmpGarrisonHolder)
				cmpGarrisonHolder.AllowGarrisoning(false,"UnitMotionFlying");
			this.pitch = 0;
			// Accelerate forwards
			this.speed = Math.min(this.template.MaxSpeed, this.speed + turnLength * this.template.AccelRate);
			canTurn = false;
			// Clamp to ground if below it, or descend if above
			if (pos.y < ground)
				pos.y = ground;
			else if (pos.y > ground)
				pos.y = Math.max(ground, pos.y - turnLength * this.template.ClimbRate);
		}
		else
		{
			this.onGround = false;
			// Climb/sink to max height above ground
			this.speed = Math.min(this.template.MaxSpeed, this.speed + turnLength * this.template.AccelRate);
			var targetHeight = ground + (+this.template.FlyingHeight);
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

	// If we're in range of the target then tell people that we've reached it
	// (TODO: quantisation breaks this)
	var distFromTarget = Math.sqrt(Math.pow(this.targetX - pos.x, 2) + Math.pow(this.targetZ - pos.z, 2));
	if (!this.reachedTarget && this.targetMinRange <= distFromTarget && distFromTarget <= this.targetMaxRange)
	{
		this.reachedTarget = true;
		Engine.PostMessage(this.entity, MT_MotionChanged, { "starting": false, "error": false });
	}

	// If we're facing away from the target, and are still fairly close to it,
	// then carry on going straight so we overshoot in a straight line
	var isBehindTarget = ((this.targetX - pos.x) * Math.sin(angle) + (this.targetZ - pos.z) * Math.cos(angle) < 0);
	// Overshoot the target: carry on straight
	if (isBehindTarget && distFromTarget < this.template.MaxSpeed * this.template.OvershootTime)
		canTurn = false;

	if (canTurn)
	{
		// Turn towards the target
		var targetAngle = Math.atan2(this.targetX - pos.x, this.targetZ - pos.z);
		var delta = targetAngle - angle;
		// Wrap delta to -pi..pi
		delta = (delta + Math.PI) % (2*Math.PI); // range -2pi..2pi
		if (delta < 0) delta += 2*Math.PI; // range 0..2pi
		delta -= Math.PI; // range -pi..pi
		// Clamp to max rate
		var deltaClamped = Math.min(Math.max(delta, -this.template.TurnRate * turnLength), this.template.TurnRate * turnLength);
		// Calculate new orientation, in a peculiar way in order to make sure the
		// result gets close to targetAngle (rather than being n*2*pi out)
		newangle = targetAngle + deltaClamped - delta;
		if (newangle - angle > Math.PI / 18)
			this.roll = Math.PI / 9;
		else if (newangle - angle < -Math.PI / 18)
			this.roll = - Math.PI / 9;
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
	var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return false;

	var targetPos = cmpTargetPosition.GetPosition2D();

	this.hasTarget = true;
	this.reachedTarget = false;
	this.targetX = targetPos.x;
	this.targetZ = targetPos.y;
	this.targetMinRange = minRange;
	this.targetMaxRange = maxRange;

	return true;
};

UnitMotionFlying.prototype.IsInPointRange = function(x, y, minRange, maxRange)
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var pos = cmpPosition.GetPosition2D();

	var distFromTarget = Math.sqrt(Math.pow(x - pos.x, 2) + Math.pow(y - pos.y, 2));
	if (minRange <= distFromTarget && distFromTarget <= maxRange)
		return true;

	return false;
};

UnitMotionFlying.prototype.IsInTargetRange = function(target, minRange, maxRange)
{
	var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return false;

	var targetPos = cmpTargetPosition.GetPosition2D();

	return this.IsInPointRange(targetPos.x, targetPos.y, minRange, maxRange);
};

UnitMotionFlying.prototype.GetWalkSpeed = function()
{
	return +this.template.MaxSpeed;
};

UnitMotionFlying.prototype.SetSpeed = function()
{
	// ignore this, the speed is always the walk speed
};

UnitMotionFlying.prototype.GetRunSpeed = function()
{
	return this.GetWalkSpeed();
};

UnitMotionFlying.prototype.GetCurrentSpeed = function()
{
	return this.speed;
};

UnitMotionFlying.prototype.GetPassabilityClassName = function()
{
	return this.template.PassabilityClass;
};

UnitMotionFlying.prototype.GetPassabilityClass = function()
{
	return this.passabilityClass;
};

UnitMotionFlying.prototype.FaceTowardsPoint = function(x, z)
{
	// Ignore this - angle is controlled by the target-seeking code instead
};

UnitMotionFlying.prototype.SetFacePointAfterMove = function()
{
	// Ignore this - angle is controlled by the target-seeking code instead
};

UnitMotionFlying.prototype.StopMoving = function()
{
	//Invert
	if (!this.waterDeath)
		this.landing = !this.landing;

};

UnitMotionFlying.prototype.SetDebugOverlay = function(enabled)
{
};

Engine.RegisterComponentType(IID_UnitMotion, "UnitMotionFlying", UnitMotionFlying);
