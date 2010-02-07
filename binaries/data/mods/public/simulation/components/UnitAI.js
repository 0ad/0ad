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

	this.nextAnimation = undefined;
};

UnitAI.prototype.OnDestroy = function()
{
	if (this.attackTimer)
	{
		cmpTimer.CancelTimer(this.attackTimer);
		this.attackTimer = undefined;
	}
};

UnitAI.prototype.OnTurnStart = function()
{
	if (this.nextAnimation)
	{
		this.SelectAnimation(this.nextAnimation.name, this.nextAnimation.once, this.nextAnimation.speed);
		this.nextAnimation = undefined;
	}
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

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	// Cancel any previous attack timer
	if (this.attackTimer)
		cmpTimer.CancelTimer(this.attackTimer);

	// TODO: start the attack animation here
	
	// TODO: should check the range and move closer before attempting to attack

	// Perform the attack after the prepare time, but not before the previous attack's recharge
	var timers = cmpAttack.GetTimers();
	var time = Math.max(timers.prepare, this.attackRechargeTime - cmpTimer.GetTime());

	var data = { "target": target, "timers": timers };
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", time, data);

	this.state = STATE_ATTACKING;
};

//// Message handlers ////

UnitAI.prototype.OnMotionStopped = function()
{
	this.SelectAnimationDelayed("idle");
};

//// Private functions ////

UnitAI.prototype.SelectAnimation = function(name, once, speed)
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SelectAnimation(name, once, speed);

	this.nextAnimation = undefined;
};

UnitAI.prototype.SelectAnimationDelayed = function(name, once, speed)
{
	this.nextAnimation = { "name": name, "once": once, "speed": speed };
}

UnitAI.prototype.MoveToTarget = function(target)
{
	var cmpPositionTarget = Engine.QueryInterface(target, IID_Position);
	if (!cmpPositionTarget || !cmpPositionTarget.IsInWorld())
		return;

	var cmpMotion = Engine.QueryInterface(this.entity, IID_UnitMotion);

	this.SelectAnimation("walk", false, cmpMotion.GetSpeed());

	var pos = cmpPositionTarget.GetPosition();
	cmpMotion.MoveToPoint(pos.x, pos.z, 0, 1);
};

UnitAI.prototype.AttackTimeout = function(data)
{
	// If we stopped attacking before this timeout, then don't do any processing here
	if (this.state != STATE_ATTACKING)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);

	// Check if we can still reach the target
	var rangeStatus = cmpAttack.CheckRange(data.target);
	if (rangeStatus.error)
	{
		if (rangeStatus.error == "out-of-range")
		{
			// Out of range => need to move closer
			this.MoveToTarget(data.target);
			// Try again in a couple of seconds
			// (TODO: ought to have a cleverer way of detecting once we're back in range)
			this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", 2000, data);
			return;
		}

		// Otherwise it's impossible to reach the target, so give up
		// and switch back to idle
		this.state = STATE_IDLE;
		this.SelectAnimation("idle");
		return;
	}

	// Play the attack animation
	this.SelectAnimationDelayed("melee", false, 1);

	// Hit the target
	cmpAttack.PerformAttack(data.target);

	// Set a timer to hit the target again
	this.attackRechargeTime = cmpTimer.GetTime() + data.timers.recharge;
	this.attackTimer = cmpTimer.SetTimeout(this.entity, IID_UnitAI, "AttackTimeout", data.timers.repeat, data);
};

Engine.RegisterComponentType(IID_UnitAI, "UnitAI", UnitAI);
