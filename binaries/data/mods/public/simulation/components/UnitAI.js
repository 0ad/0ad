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

UnitAI.prototype.Schema =
	"<a:help>Controls the unit's movement, attacks, etc, in response to commands from the player.</a:help>" +
	"<a:example/>" +
	"<element name='NaturalBehaviour' a:help='Behaviour of the unit in the absence of player commands (intended for animals)'>" + // TODO: implement this
		"<choice>" +
			"<value a:help='Will actively attack any unit it encounters, even if not threatened'>violent</value>" +
			"<value a:help='Will attack nearby units if it feels threatened (if they linger within LOS for too long)'>aggressive</value>" +
			"<value a:help='Will attack nearby units if attacked'>defensive</value>" +
			"<value a:help='Will never attack units'>passive</value>" +
			"<value a:help='Will never attack units. Will typically attempt to flee for short distances when units approach'>skittish</value>" +
		"</choice>" +
	"</element>";

UnitAI.prototype.Init = function()
{
	this.state = STATE_IDLE;

	// The earliest time at which we'll have 'recovered' from the previous attack, and
	// can start preparing a new attack
	this.attackRechargeTime = 0;
	// Timer for AttackTimeout
	this.attackTimer = undefined;
	// Current attack type
	this.attackType = undefined;
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

	if (cmpMotion.MoveToPoint(x, z))
	{
		this.state = STATE_WALKING;
		PlaySound("walk", this.entity);
	}
	else
	{
		this.state = STATE_IDLE;
	}
};

UnitAI.prototype.WalkToTarget = function(target)
{
	var cmpPosition = Engine.QueryInterface(target, IID_Position);
	if (!cmpPosition)
		return;

	if (!cmpPosition.IsInWorld())
		return;

	var pos = cmpPosition.GetPosition();
	this.Walk(pos.x, pos.z);
}

UnitAI.prototype.Attack = function(target)
{
	// Verify that we're able to respond to Attack commands
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
	{
		this.WalkToTarget(target);
		return;
	}

	// TODO: verify that this is a valid target

	var type = cmpAttack.GetBestAttack();
	if (!type)
	{
		this.WalkToTarget(target);
		return;
	}

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.attackType = type;
	this.attackTarget = target;
	this.state = STATE_ATTACKING;
	if (!this.MoveToTarget(target, cmpAttack.GetRange(type)))
	{
		// We're in range already, do the attack
		// (TODO: this could also happen if we couldn't move anywhere)
		this.StartAttack();
	}
	// else we've started moving and the attack will start in OnMotionChanged
};

UnitAI.prototype.Repair = function(target)
{
	// Verify that we're able to respond to Repair commands
	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);
	if (!cmpBuilder)
	{
		this.WalkToTarget(target);
		return;
	}

	// TODO: verify that this is a valid target

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.repairTarget = target;
	this.state = STATE_REPAIRING;
	if (!this.MoveToTarget(target, cmpBuilder.GetRange()))
	{
		// We're in range already, do the repairing
		// (TODO: this could also happen if we couldn't move anywhere)
		this.StartRepair();
	}
	// else we've started moving and the repair will start in OnMotionChanged
};

UnitAI.prototype.Gather = function(target)
{
	// Verify that we're able to respond to Gather commands
	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (!cmpResourceGatherer)
	{
		this.WalkToTarget(target);
		return;
	}

	// Verify that we can gather from this target
	if (!cmpResourceGatherer.GetTargetGatherRate(target))
	{
		this.WalkToTarget(target);
		return;
	}

	// TODO: verify that this is a valid target

	// Stop any previous action timers
	this.CancelTimers();

	// Remember the target, and start moving towards it
	this.gatherTarget = target;
	this.state = STATE_GATHERING;
	if (!this.MoveToTarget(target, cmpResourceGatherer.GetRange()))
	{
		// We're in range already, do the gathering
		// (TODO: this could also happen if we couldn't move anywhere)
		this.StartGather();
	}
	// else we've started moving and the gather will start in OnMotionChanged
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

			if (this.MoveIntoRange(IID_Attack, this.attackTarget, this.attackType))
				return;

			// In range, so perform the attack
			this.StartAttack();
		}
		else if (this.state == STATE_REPAIRING)
		{
			// We were repairing, and have stopped moving
			// => check if we can still reach the target now

			if (this.MoveIntoRange(IID_Builder, this.repairTarget))
				return;

			// In range, so perform the repairing
			this.StartRepair();
		}
		else if (this.state == STATE_GATHERING)
		{
			// We were gathering, and have stopped moving
			// => check if we can still reach the target now

			if (this.MoveIntoRange(IID_ResourceGatherer, this.gatherTarget))
				return;

			// In range, so perform the gathering
			this.StartGather();
		}
	}
};

//// Private functions ////

UnitAI.prototype.StartAttack = function()
{
	// Perform the attack after the prepare time but not before the previous attack's recharge
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	var timers = cmpAttack.GetTimers(this.attackType);
	var time = Math.max(timers.prepare, this.attackRechargeTime - cmpTimer.GetTime());
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", time, {});

	// Start the attack animation and sound, but synced to the timers
	this.SelectAnimation("melee", false, 1.0, "attack");
	this.SetAnimationSync(time, timers.repeat);
	// TODO: this drifts since the sim is quantised to sim turns and these timers aren't
	// TODO: we should probably only bother syncing projectile attacks, not melee
};

UnitAI.prototype.StartRepair = function()
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.repairTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "RepairTimeout", 1000, {});

	// Start the repair/build animation and sound
	this.SelectAnimation("build", false, 1.0, "build");
};

UnitAI.prototype.StartGather = function()
{
	var cmpResourceSupply = Engine.QueryInterface(this.gatherTarget, IID_ResourceSupply);
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	// Get the animation/sound type name
	var type = cmpResourceSupply.GetType();
	var typename = "gather_" + (type.specific || type.generic);

	this.gatherTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "GatherTimeout", 1000, {"typename": typename});

	// Start the gather animation and sound
	this.SelectAnimation(typename, false, 1.0, typename);
};

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
 * Tries to move into range of the target.
 * Returns true if the unit has started walking or on pathing failure, false if already in range.
 */
UnitAI.prototype.MoveIntoRange = function(iid, target, type)
{
	var cmpRanged = Engine.QueryInterface(this.entity, iid);
	var range = cmpRanged.GetRange(type);

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (cmpMotion.IsInAttackRange(target, range.min, range.max))
		return false;

	// Out of range => need to move closer
	// (The target has probably moved while we were chasing it)
	if (this.MoveToTarget(target, range))
		return true;

	// If it's impossible to reach the target, give up
	// and switch back to idle
	this.state = STATE_IDLE;
	this.SelectAnimation("idle");
	return true;
};

UnitAI.prototype.SelectAnimation = function(name, once, speed, sound)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	var soundgroup;
	if (sound)
	{
		var cmpSound = Engine.QueryInterface(this.entity, IID_Sound);
		if (cmpSound)
			soundgroup = cmpSound.GetSoundGroup(sound);
	}

	// Set default values if unspecified
	if (typeof once == "undefined")
		once = false;
	if (typeof speed == "undefined")
		speed = 1.0;
	if (typeof soundgroup == "undefined")
		soundgroup = "";

	cmpVisual.SelectAnimation(name, once, speed, soundgroup);
};

UnitAI.prototype.SetAnimationSync = function(actiontime, repeattime)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SetAnimationSyncRepeat(repeattime);
	cmpVisual.SetAnimationSyncOffset(actiontime);
};

/**
 * Tries to move to the specified range of the target.
 * This might synchronously trigger a MotionChanged message.
 * Returns true if the unit has started walking, false on error or if already in range.
 */
UnitAI.prototype.MoveToTarget = function(target, range)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	return cmpMotion.MoveToAttackRange(target, range.min, range.max);
};

UnitAI.prototype.AttackTimeout = function(data, lateness)
{
	// If we stopped attacking before this timeout, then don't do any processing here
	if (this.state != STATE_ATTACKING)
		return;

	// Check if we can still reach the target
	if (this.MoveIntoRange(IID_Attack, this.attackTarget, this.attackType))
		return;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	// Hit the target
	cmpAttack.PerformAttack(this.attackType, this.attackTarget);

	// Set a timer to hit the target again

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	var timers = cmpAttack.GetTimers(this.attackType);
	this.attackRechargeTime = cmpTimer.GetTime() + timers.recharge;
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", timers.repeat - lateness, data);
};

UnitAI.prototype.RepairTimeout = function(data, lateness)
{
	// If we stopped repairing before this timeout, then don't do any processing here
	if (this.state != STATE_REPAIRING)
		return;

	// Check if we can still reach the target
	if (this.MoveIntoRange(IID_Builder, this.repairTarget))
		return;

	var cmpBuilder = Engine.QueryInterface(this.entity, IID_Builder);

	// Repair/build the target
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
	this.repairTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "RepairTimeout", 1000 - lateness, data);
};

UnitAI.prototype.GatherTimeout = function(data, lateness)
{
	// If we stopped gathering before this timeout, then don't do any processing here
	if (this.state != STATE_GATHERING)
		return;

	// Check if we can still reach the target
	if (this.MoveIntoRange(IID_ResourceGatherer, this.gatherTarget))
		return;

	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);

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
	this.gatherTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "GatherTimeout", 1000 - lateness, data);
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
