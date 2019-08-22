function StatusEffectsReceiver() {}

StatusEffectsReceiver.prototype.Init = function()
{
	this.activeStatusEffects = {};
};

// Called by attacking effects.
StatusEffectsReceiver.prototype.GiveStatus = function(effectData, attacker, attackerOwner, bonusMultiplier)
{
	for (let effect in effectData)
		this.AddStatus(effect, effectData[effect]);

	// TODO: implement loot / resistance.

	return { "inflictedStatuses": Object.keys(effectData) };
};

StatusEffectsReceiver.prototype.AddStatus = function(statusName, data)
{
	if (this.activeStatusEffects[statusName])
		return;

	this.activeStatusEffects[statusName] = {};
	let status = this.activeStatusEffects[statusName];
	status.duration = +data.Duration;
	status.interval = +data.Interval;
	status.damage = +data.Damage;
	status.timeElapsed = 0;
	status.firstTime = true;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	status.timer = cmpTimer.SetInterval(this.entity, IID_StatusEffectsReceiver, "ExecuteEffect", 0, +status.interval, statusName);
};

StatusEffectsReceiver.prototype.RemoveStatus = function(statusName) {
	if (!this.activeStatusEffects[statusName])
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.activeStatusEffects[statusName].timer);
	this.activeStatusEffects[statusName] = undefined;
};

StatusEffectsReceiver.prototype.ExecuteEffect = function(statusName, lateness)
{
	let status = this.activeStatusEffects[statusName];
	if (!status)
		return;

	if (status.firstTime)
	{
		status.firstTime = false;
		status.timeElapsed += lateness;
	}
	else
		status.timeElapsed += status.interval + lateness;

	Attacking.HandleAttackEffects(statusName, { "Damage": { [statusName]: status.damage } }, this.entity, -1, -1);

	if (status.timeElapsed >= status.duration)
		this.RemoveStatus(statusName);
};

Engine.RegisterComponentType(IID_StatusEffectsReceiver, "StatusEffectsReceiver", StatusEffectsReceiver);
