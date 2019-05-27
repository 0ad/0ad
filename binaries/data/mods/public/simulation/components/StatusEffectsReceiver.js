function StatusEffectsReceiver() {}

StatusEffectsReceiver.prototype.Init = function()
{
	this.activeStatusEffects = {};
};

StatusEffectsReceiver.prototype.InflictEffects = function(statusEffects)
{
	for (let effect in statusEffects)
		this.InflictEffect(effect, statusEffects[effect]);
};

StatusEffectsReceiver.prototype.InflictEffect = function(statusName, data)
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

StatusEffectsReceiver.prototype.RemoveEffect = function(statusName) {
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

	let cmpDamage = Engine.QueryInterface(SYSTEM_ENTITY, IID_Damage);

	cmpDamage.CauseDamage({
		"strengths": { [statusName]: status.damage },
		"target": this.entity,
		"attacker": -1,
		"multiplier": 1,
		"type": statusName,
		"attackerOwner": -1
	});

	if (status.timeElapsed >= status.duration)
		this.RemoveEffect(statusName);
};

Engine.RegisterComponentType(IID_StatusEffectsReceiver, "StatusEffectsReceiver", StatusEffectsReceiver);
