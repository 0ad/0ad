/*

This is currently just a very simplistic state machine that lets units be commanded around
and then autonomously carry out the orders. It might need to be entirely redesigned.

*/

const STATE_IDLE = 0;
const STATE_WALKING = 1;
const STATE_ATTACKING = 2;
const STATE_REPAIRING = 3;
const STATE_GATHERING = 4;

/* Attack process:
 *   When starting attack:
 *     Activate attack animation (with appropriate repeat speed and offset)
 *     Set this.attackTimer to run at maximum of:
 *       GetTimers().prepare msec from now
 *       this.attackRechargeTime
 *     Loop:
 *       Wait for the timer
 *       Perform the attack
 *       Set this.attackRechargeTime to now plus GetTimers().recharge
 *       Set this.attackTimer to run after GetTimers().repeat
 *   At any point it's safe to cancel the attack and switch to a different action
 *   (The rechargeTime is to prevent people spamming the attack command and getting
 *   faster-than-normal attacks)
 */

/* Repeat/Gather process is about the same, except with less synchronisation - the action
 * is just performed 1sec after initiated, and then repeated every 1sec.
 * (TODO: it'd be nice to avoid most of the duplication between Attack and Repeat and Gather code)
 */

function UnitAI() {}

UnitAI.prototype.Init = function()
{
	this.state = STATE_IDLE;

	// The earliest time at which we'll have 'recovered' from the previous attack, and
	// can start preparing a new attack
	this.attackRechargeTime = 0;
	// Timer for AttackTimeout
	this.attackTimer = undefined;
	// Current target entity ID
	this.attackTarget = undefined;

	// Timer for RepairTimeout
	this.repairTimer = undefined;
	// Current target entity ID
	this.repairTarget = undefined;

	// Timer for GatherTimeout
	this.gatherTimer = undefined;
	// Current target entity ID
	this.gatherTarget = undefined;
};

//// Interface functions ////

UnitAI.prototype.Walk = function(x, z)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!cmpMotion)
		return;

	this.SelectAnimation("walk", false, cmpMotion.GetSpeed());
	PlaySound("walk", this.entity);

	cmpMotion.MoveToPoint(x, z, 0, 0);

	this.state = STATE_WALKING;
};

UnitAI.prototype.Attack = function(target)
{
	// Verify that we're able to respond to Attack commands
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	// TODO: verify that this is a valid target

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.attackTarget = target;
	this.MoveToTarget(target, cmpAttack.GetRange());
	this.state = STATE_ATTACKING;
};

UnitAI.prototype.Repair = function(target)
{
	// Verify that we're able to respond to Repair commands
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	if (!cmpBuilder)
		return;

	// TODO: verify that this is a valid target

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.repairTarget = target;
	this.MoveToTarget(target, cmpBuilder.GetRange());
	this.state = STATE_REPAIRING;
};

UnitAI.prototype.Gather = function(target)
{
	// Verify that we're able to respond to Gather commands
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
		return;

	// TODO: verify that this is a valid target

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.gatherTarget = target;
	this.MoveToTarget(target, cmpResourceGatherer.GetRange());
	this.state = STATE_GATHERING;
};

//// Message handlers ////

UnitAI.prototype.OnDestroy = function()
{
	// Clean up any timers that are now obsolete
	this.CancelTimers();
};

UnitAI.prototype.OnMotionChanged = function(msg)
{
	if (msg.speed)
	{
		// Started moving
		// => play the appropriate animation
		this.SelectAnimation("walk", false, msg.speed);
	}
	else
	{
		if (this.state == STATE_WALKING)
		{
			// Stopped walking
			this.state = STATE_IDLE;
			this.SelectAnimation("idle");
		}
		else if (this.state == STATE_ATTACKING)
		{
			// We were attacking, and have stopped moving
			// => check if we can still reach the target now

			if (!this.MoveIntoAttackRange())
				return;

			// In range, so perform the attack,
			// after the prepare time but not before the previous attack's recharge

			var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

			var timers = cmpAttack.GetTimers();
			var time = Math.max(timers.prepare, this.attackRechargeTime - cmpTimer.GetTime());
			this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", time, {});

			// Start the idle animation before we switch to the attack
			this.SelectAnimation("idle");
		}
		else if (this.state == STATE_REPAIRING)
		{
			// We were repairing, and have stopped moving
			// => check if we can still reach the target now

			if (!this.MoveIntoRepairRange())
				return;

			// In range, so perform the repairing

			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			this.repairTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "RepairTimeout", 1000, {});

			// Start the repair/build animation
			this.SelectAnimation("build");
		}
		else if (this.state == STATE_GATHERING)
		{
			// We were gathering, and have stopped moving
			// => check if we can still reach the target now

			if (!this.MoveIntoGatherRange())
				return;

			// In range, so perform the gathering

			var cmpResourceSupply = Engine.QueryInterface(this.gatherTarget, IID_ResourceSupply);
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

			// Get the animation/sound type name
			var type = cmpResourceSupply.GetType();
			var typename = "gather_" + (type.specific || type.generic);

			this.gatherTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "GatherTimeout", 1000, {"typename": typename});

			// Start the gather animation
			this.SelectAnimation(typename);
		}
	}
};

//// Private functions ////

function hypot2(x, y)
{
	return x*x + y*y;
}

UnitAI.prototype.CheckRange = function(target, range)
{
	// Target must be in the world
	var cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
	if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
		return { "error": "not-in-world" };

	// We must be in the world
	var cmpPositionSelf = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPositionSelf || !cmpPositionSelf.IsInWorld())
		return { "error": "not-in-world" };

	// Target must be within range
	var posTarget = cmpPositionTarget.GetPosition();
	var posSelf = cmpPositionSelf.GetPosition();
	var dist2 = hypot2(posTarget.x - posSelf.x, posTarget.z - posSelf.z);
	// TODO: ought to be distance to closest point in footprint, not to center
	// The +4 is a hack to give a ~1 tile tolerance, because the pathfinder doesn't
	// always get quite close enough to the target
	if (dist2 > (range.max+4)*(range.max+4))
		return { "error": "out-of-range" };

	return {};
}

UnitAI.prototype.CancelTimers = function()
{
	if (this.attackTimer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.attackTimer);
		this.attackTimer = undefined;
	}

	if (this.repairTimer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.repairTimer);
		this.repairTimer = undefined;
	}

	if (this.gatherTimer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.gatherTimer);
		this.gatherTimer = undefined;
	}
};

/**
 * Tries to move into range of the attack target.
 * Returns true if it's already in range.
 */
UnitAI.prototype.MoveIntoAttackRange = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	var range = cmpAttack.GetRange();

	var rangeStatus = this.CheckRange(this.attackTarget, range);
	if (rangeStatus.error)
	{
		if (rangeStatus.error == "out-of-range")
		{
			// Out of range => need to move closer
			// (The target has probably moved while we were chasing it)
			this.MoveToTarget(this.attackTarget, range);
			return false;
		}

		// Otherwise it's impossible to reach the target, so give up
		// and switch back to idle
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return false;
	}
	
	return true;
};

UnitAI.prototype.MoveIntoRepairRange = function()
{
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	var range = cmpBuilder.GetRange();

	var rangeStatus = this.CheckRange(this.repairTarget, range);
	if (rangeStatus.error)
	{
		if (rangeStatus.error == "out-of-range")
		{
			// Out of range => need to move closer
			// (The target has probably moved while we were chasing it)
			this.MoveToTarget(this.repairTarget, range);
			return false;
		}

		// Otherwise it's impossible to reach the target, so give up
		// and switch back to idle
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return false;
	}
	
	return true;
};

UnitAI.prototype.MoveIntoGatherRange = function()
{
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	var range = cmpResourceGatherer.GetRange();

	var rangeStatus = this.CheckRange(this.gatherTarget, range);
	if (rangeStatus.error)
	{
		if (rangeStatus.error == "out-of-range")
		{
			// Out of range => need to move closer
			// (The target has probably moved while we were chasing it)
			this.MoveToTarget(this.gatherTarget, range);
			return false;
		}

		// Otherwise it's impossible to reach the target, so give up
		// and switch back to idle
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return false;
	}
	
	return true;
};

// TODO: refactor all this repetitive code

UnitAI.prototype.SelectAnimation = function(name, once, speed)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SelectAnimation(name, once, speed);
};

UnitAI.prototype.MoveToTarget = function(target, range)
{
	var cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
	if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
		return;

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);

	var pos = cmpPositionTarget.GetPosition();
	cmpMotion.MoveToPoint(pos.x, pos.z, range.min, range.max);
};

UnitAI.prototype.AttackTimeout = function(data)
{
	// If we stopped attacking before this timeout, then don't do any processing here
	if (this.state != STATE_ATTACKING)
		return;

	// Check if we can still reach the target
	if (!this.MoveIntoAttackRange())
		return;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	// Play the attack animation
	this.SelectAnimation("melee", false, 1);
	// Play the sound
	// TODO: these sounds should be triggered by the animation instead
	PlaySound("attack", this.entity);

	// Hit the target
	cmpAttack.PerformAttack(this.attackTarget);

	// Set a timer to hit the target again

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	var timers = cmpAttack.GetTimers();
	this.attackRechargeTime = cmpTimer.GetTime() + timers.recharge;
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", timers.repeat, data);
};

UnitAI.prototype.RepairTimeout = function(data)
{
	// If we stopped repairing before this timeout, then don't do any processing here
	if (this.state != STATE_REPAIRING)
		return;

	// Check if we can still reach the target
	if (!this.MoveIntoRepairRange())
		return;

	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);

	// Play the sound
	PlaySound("build", this.entity);

	// Repair/build the target
	// TODO: these sounds should be triggered by the animation instead
	var status = cmpBuilder.PerformBuilding(this.repairTarget);

	// If the target is fully built and repaired, then stop and go back to idle
	if (status.finished)
	{
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return;
	}

	// Set a timer to gather again

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.repairTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "RepairTimeout", 1000, data);
};

UnitAI.prototype.GatherTimeout = function(data)
{
	// If we stopped gathering before this timeout, then don't do any processing here
	if (this.state != STATE_GATHERING)
		return;

	// Check if we can still reach the target
	if (!this.MoveIntoGatherRange())
		return;

	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);

	// Play the gather_* sound
	// TODO: these sounds should be triggered by the animation instead
	PlaySound(data.typename, this.entity);

	// Gather from the target
	var status = cmpResourceGatherer.PerformGather(this.gatherTarget);

	// If the resource is exhausted, then stop and go back to idle
	if (status.exhausted)
	{
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return;
	}

	// Set a timer to gather again

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.gatherTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "GatherTimeout", 1000, data);
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
