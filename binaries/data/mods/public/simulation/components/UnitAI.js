/*

This is currently just a very simplistic state machine that lets units be commanded around
and then autonomously carry out the orders.

*/

const STATE_IDLE = 0;
const STATE_WALKING = 1;
const STATE_ATTACKING = 2;

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
};

//// Interface functions ////

UnitAI.prototype.Walk = function(x, z)
{
	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);
	if (!cmpMotion)
		return;

	this.SelectAnimation("walk", false, cmpMotion.GetSpeed());

	cmpMotion.MoveToPoint(x, z, 0, 0);

	this.state = STATE_WALKING;
};

UnitAI.prototype.Attack = function(target)
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	// Remember the target, and start moving towards it
	this.attackTarget = target;
	this.MoveToTarget(this.attackTarget);
	this.state = STATE_ATTACKING;

	// Cancel any previous attack timer
	if (this.attackTimer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.attackTimer);
		this.attackTimer = undefined;
	}
};

//// Message handlers ////

UnitAI.prototype.OnDestroy = function()
{
	if (this.attackTimer)
	{
		cmpTimer.CancelTimer(this.attackTimer);
		this.attackTimer = undefined;
	}
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
	}
};

//// Private functions ////

/**
 * Tries to move into range of the attack target.
 * Returns true if it's already in range.
 */
UnitAI.prototype.MoveIntoAttackRange = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	var rangeStatus = cmpAttack.CheckRange(this.attackTarget);
	if (rangeStatus.error)
	{
		if (rangeStatus.error == "out-of-range")
		{
			// Out of range => need to move closer
			// (The target has probably moved while we were chasing it)
			this.MoveToTarget(this.attackTarget);
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

UnitAI.prototype.SelectAnimation = function(name, once, speed)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SelectAnimation(name, once, speed);
};

UnitAI.prototype.MoveToTarget = function(target)
{
	var cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
	if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
		return;

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);

	var pos = cmpPositionTarget.GetPosition();
	cmpMotion.MoveToPoint(pos.x, pos.z, 0, 1);
};

UnitAI.prototype.AttackTimeout = function(data)
{
	// If we stopped attacking before this timeout, then don't do any processing here
	if (this.state != STATE_ATTACKING)
		return;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	// Check if we can still reach the target
	if (!this.MoveIntoAttackRange())
		return;

	// Play the attack animation
	this.SelectAnimation("melee", false, 1);

	// Hit the target
	cmpAttack.PerformAttack(this.attackTarget);

	// Set a timer to hit the target again

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	var timers = cmpAttack.GetTimers();
	this.attackRechargeTime = cmpTimer.GetTime() + timers.recharge;
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", timers.repeat, data);
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
