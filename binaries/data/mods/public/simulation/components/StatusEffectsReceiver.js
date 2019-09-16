function StatusEffectsReceiver() {}

StatusEffectsReceiver.prototype.Init = function()
{
	this.activeStatusEffects = {};
};

StatusEffectsReceiver.prototype.GetActiveStatuses = function()
{
	return this.activeStatusEffects;
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
	Object.assign(status, data);
	status.Interval = +data.Interval;
	status.TimeElapsed = 0;
	status.FirstTime = true;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	status.Timer = cmpTimer.SetInterval(this.entity, IID_StatusEffectsReceiver, "ExecuteEffect", 0, +status.Interval, statusName);
};

StatusEffectsReceiver.prototype.RemoveStatus = function(statusName)
{
	if (!this.activeStatusEffects[statusName])
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.activeStatusEffects[statusName].Timer);
	delete this.activeStatusEffects[statusName];
};

StatusEffectsReceiver.prototype.ExecuteEffect = function(statusName, lateness)
{
	let status = this.activeStatusEffects[statusName];
	if (!status)
		return;

	if (status.FirstTime)
	{
		status.FirstTime = false;
		status.TimeElapsed += lateness;
	}
	else
		status.TimeElapsed += status.Interval + lateness;

	Attacking.HandleAttackEffects(statusName, status, this.entity, -1, -1);

	if (status.Duration && status.TimeElapsed >= +status.Duration)
		this.RemoveStatus(statusName);
};

Engine.RegisterComponentType(IID_StatusEffectsReceiver, "StatusEffectsReceiver", StatusEffectsReceiver);
