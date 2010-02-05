function Attack() {}

Attack.prototype.Init = function()
{
};

/*
 * TODO: to handle secondary attacks in the future, what we might do is
 * add a 'mode' parameter to most of these functions, to indicate which
 * attack mode we're trying to use, and some other function that allows
 * UnitAI to pick the best attack mode (based on range, damage, etc)
 */

Attack.prototype.GetTimers = function()
{
	var prepare = +(this.template.PrepareTime || 0);
	var repeat = +(this.template.RepeatTime || 1000);
	return { "prepare": prepare, "repeat": repeat, "recharge": repeat - prepare };
};

Attack.prototype.GetAttackStrengths = function()
{
	// Convert attack values to numbers, default 0 if unspecified
	return {
		hack: +(this.template.Hack || 0),
		pierce: +(this.template.Pierce || 0),
		crush: +(this.template.Crush || 0)
	};
};

function hypot2(x, y)
{
	return x*x + y*y;
}

Attack.prototype.CheckRange = function(target)
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
	var maxrange = +this.template.Range;
	if (dist2 > maxrange*maxrange)
		return { "error": "out-of-range", "maxrange": maxrange };

	return {};
}

/**
 * Attack the target entity. This should only be called after a successful CheckRange,
 * and should only be called after GetTimers().repeat msec has passed since the last
 * call to PerformAttack.
 */
Attack.prototype.PerformAttack = function(target)
{
	var strengths = this.GetAttackStrengths();

	// Inflict damage on the target
	var cmpDamageReceiver = Engine.QueryInterface(target, IID_DamageReceiver);
	if (!cmpDamageReceiver)
		return;
	cmpDamageReceiver.TakeDamage(strengths.hack, strengths.pierce, strengths.crush);
};

Engine.RegisterComponentType(IID_Attack, "Attack", Attack);
