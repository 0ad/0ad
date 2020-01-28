function StatusEffectsReceiver() {}

StatusEffectsReceiver.prototype.DefaultInterval = 1000;

/**
 * Initialises the status effects.
 */
StatusEffectsReceiver.prototype.Init = function()
{
	this.activeStatusEffects = {};
};

/**
 * Which status effects are active on this entity.
 *
 * @return {Object} - An object containing the status effects which currently affect the entity.
 */
StatusEffectsReceiver.prototype.GetActiveStatuses = function()
{
	return this.activeStatusEffects;
};

/**
 * Called by Attacking effects. Adds status effects for each entry in the effectData.
 *
 * @param {Object} effectData - An object containing the status effects to give to the entity.
 * @param {number} attacker - The entity ID of the attacker.
 * @param {number} attackerOwner - The player ID of the attacker.
 * @param {number} bonusMultiplier - A value to multiply the damage with (not implemented yet for SE).
 *
 * @return {Object} - The names of the status effects which were processed.
 */
StatusEffectsReceiver.prototype.ApplyStatus = function(effectData, attacker, attackerOwner, bonusMultiplier)
{
	let attackerData = { "entity": attacker, "owner": attackerOwner };
	for (let effect in effectData)
		this.AddStatus(effect, effectData[effect], attackerData);

	// TODO: implement loot / resistance.

	return { "inflictedStatuses": Object.keys(effectData) };
};

/**
 * Adds a status effect to the entity.
 *
 * @param {string} statusName - The name of the status effect.
 * @param {object} data - The various effects and timings.
 */
StatusEffectsReceiver.prototype.AddStatus = function(statusName, data, attackerData)
{
	if (this.activeStatusEffects[statusName])
	{
		// TODO: implement different behaviour when receiving the same status multiple times.
		// For now, these are ignored.
		return;
	}

	this.activeStatusEffects[statusName] = {};
	let status = this.activeStatusEffects[statusName];
	Object.assign(status, data);

	if (status.Modifiers)
	{
		let modifications = DeriveModificationsFromXMLTemplate(status.Modifiers);
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.AddModifiers(statusName, modifications, this.entity);
	}

	// With neither an interval nor a duration, there is no point in starting a timer.
	if (!status.Duration && !status.Interval)
		return;

	// We need this to prevent Status Effects from giving XP
	// to the entity that applied them.
	status.StatusEffect = true;

	// We want an interval to update the GUI to show how much time of the status effect
	// is left even if the status effect itself has no interval.
	if (!status.Interval)
		status._interval = this.DefaultInterval;

	status._timeElapsed = 0;
	status._firstTime = true;
	status.source = attackerData;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	status._timer = cmpTimer.SetInterval(this.entity, IID_StatusEffectsReceiver, "ExecuteEffect", 0, +(status.Interval || status._interval), statusName);
};

/**
 * Removes a status effect from the entity.
 *
 * @param {string} statusName - The status effect to be removed.
 */
StatusEffectsReceiver.prototype.RemoveStatus = function(statusName)
{
	let statusEffect = this.activeStatusEffects[statusName];
	if (!statusEffect)
		return;

	if (statusEffect.Modifiers)
	{
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.RemoveAllModifiers(statusName, this.entity);
	}

	if (statusEffect._timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(statusEffect._timer);
	}
	delete this.activeStatusEffects[statusName];
};

/**
 * Called by the timers. Executes a status effect.
 *
 * @param {string} statusName - The name of the status effect to be executed.
 * @param {number} lateness - The delay between the calling of the function and the actual execution (turn time?).
 */
StatusEffectsReceiver.prototype.ExecuteEffect = function(statusName, lateness)
{
	let status = this.activeStatusEffects[statusName];
	if (!status)
		return;

	if (status.Damage || status.Capture)
		Attacking.HandleAttackEffects(statusName, status, this.entity, status.source.entity, status.source.owner);

	if (!status.Duration)
		return;

	if (status._firstTime)
	{
		status._firstTime = false;
		status._timeElapsed += lateness;
	}
	else
		status._timeElapsed += +(status.Interval || status._interval) + lateness;

	if (status._timeElapsed >= +status.Duration)
		this.RemoveStatus(statusName);
};

Engine.RegisterComponentType(IID_StatusEffectsReceiver, "StatusEffectsReceiver", StatusEffectsReceiver);
