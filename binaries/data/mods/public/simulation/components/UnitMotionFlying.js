// (A serious implementation of this might want to use C++ instead of JS
// for performance; this is just for fun.)

function UnitMotionFlying() {}

UnitMotionFlying.prototype.Schema =
	"<element name='MaxSpeed'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='AccelRate'>" +
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
};

UnitMotionFlying.prototype.OnUpdate = function(msg)
{
	var turnLength = msg.turnLength;

	if (!this.hasTarget)
		return;

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var pos = cmpPosition.GetPosition();
	var angle = cmpPosition.GetRotation().y;

	var canTurn = true;

	// If we haven't reached max speed yet then we're still on the ground;
	// otherwise we're taking off or flying
	if (this.speed < this.template.MaxSpeed)
	{
		// Accelerate forwards
		this.speed = Math.min(this.template.MaxSpeed, this.speed + turnLength*this.template.AccelRate);
		canTurn = false;

		// Clamp to ground if below it, or descend if above

		var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
		var ground = cmpTerrain.GetGroundLevel(pos.x, pos.z);

		if (pos.y < ground)
			pos.y = ground;
		else if (pos.y > ground)
			pos.y = Math.max(ground, pos.y - turnLength*this.template.ClimbRate);

		cmpPosition.SetHeightFixed(pos.y);
	}
	else
	{
		// Climb/sink to max height above ground

		var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);
		var ground = cmpTerrain.GetGroundLevel(pos.x, pos.z);

		var targetHeight = ground + (+this.template.FlyingHeight);
		if (pos.y < targetHeight)
			pos.y = Math.min(targetHeight, pos.y + turnLength*this.template.ClimbRate);
		else if (pos.y > targetHeight)
			pos.y = Math.max(targetHeight, pos.y - turnLength*this.template.ClimbRate);

		cmpPosition.SetHeightFixed(pos.y);
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
	var isBehindTarget = ((this.targetX - pos.x)*Math.sin(angle) + (this.targetZ - pos.z)*Math.cos(angle) < 0);
	if (isBehindTarget && distFromTarget < this.template.MaxSpeed*this.template.OvershootTime)
	{
		// Overshoot the target: carry on straight
		canTurn = false;
	}

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
		var deltaClamped = Math.min(Math.max(delta, -this.template.TurnRate*turnLength), this.template.TurnRate*turnLength);
		// Calculate new orientation, in a peculiar way in order to make sure the
		// result gets close to targetAngle (rather than being n*2*pi out)
		angle = targetAngle + deltaClamped - delta;
	}

	pos.x += this.speed * turnLength * Math.sin(angle);
	pos.z += this.speed * turnLength * Math.cos(angle);

	cmpPosition.TurnTo(angle);
	cmpPosition.MoveTo(pos.x, pos.z);
};

UnitMotionFlying.prototype.MoveToPointRange = function(x, z, minRange, maxRange)
{
	this.hasTarget = true;
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

UnitMotionFlying.prototype.IsInTargetRange = function(target, minRange, maxRange)
{
	var cmpTargetPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpTargetPosition || !cmpTargetPosition.IsInWorld())
		return false;

	var targetPos = cmpTargetPosition.GetPosition2D();

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var pos = cmpPosition.GetPosition2D();

	var distFromTarget = Math.sqrt(Math.pow(targetPos.x - pos.x, 2) + Math.pow(targetPos.y - pos.y, 2));
	if (minRange <= distFromTarget && distFromTarget <= maxRange)
		return true;

	return false;
};

UnitMotionFlying.prototype.GetWalkSpeed = function()
{
	return +this.template.MaxSpeed;
};

UnitMotionFlying.prototype.GetRunSpeed = function()
{
	return this.GetWalkSpeed();
};

UnitMotionFlying.prototype.FaceTowardsPoint = function(x, z)
{
	// Ignore this - angle is controlled by the target-seeking code instead
};

UnitMotionFlying.prototype.StopMoving = function()
{
	// Ignore this - we can never stop moving
};

UnitMotionFlying.prototype.SetDebugOverlay = function(enabled)
{
};

Engine.RegisterComponentType(IID_UnitMotion, "UnitMotionFlying", UnitMotionFlying);
