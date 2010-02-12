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

Attack.prototype.GetRange = function()
{
	return { "max": +this.template.Range, "min": 0 };
}

/**
 * Attack the target entity. This should only be called after a successful range check,
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
