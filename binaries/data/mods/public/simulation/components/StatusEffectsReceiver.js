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
 * @return {Object} - The codes of the status effects which were processed.
 */
StatusEffectsReceiver.prototype.ApplyStatus = function(effectData, attacker, attackerOwner)
{
	for (let effect in effectData)
		this.AddStatus(effect, effectData[effect], attacker, attackerOwner);

	// TODO: implement loot?

	return { "inflictedStatuses": Object.keys(effectData) };
};

/**
 * Adds a status effect to the entity.
 *
 * @param {string} statusCode - The code of the status effect.
 * @param {Object} data - The various effects and timings.
 * @param {number} attacker - optional, the entity ID of the attacker.
 * @param {number} attackerOwner - optional, the player ID of the attacker.
 */
StatusEffectsReceiver.prototype.AddStatus = function(baseCode, data, attacker = INVALID_ENTITY, attackerOwner = INVALID_PLAYER)
{
	let statusCode = baseCode;
	if (this.activeStatusEffects[statusCode])
	{
		if (data.Stackability == "Ignore")
			return;
		if (data.Stackability == "Extend")
		{
			this.activeStatusEffects[statusCode].Duration += data.Duration;
			return;
		}
		if (data.Stackability == "Replace")
			this.RemoveStatus(statusCode);
		else if (data.Stackability == "Stack")
		{
			let i = 0;
			let temp;
			do
				temp = statusCode + "_" + i++;
			while (!!this.activeStatusEffects[temp]);
			statusCode = temp;
		}
	}

	this.activeStatusEffects[statusCode] = {
		"baseCode": baseCode
	};
	let status = this.activeStatusEffects[statusCode];
	Object.assign(status, data);

	if (status.Modifiers)
	{
		let modifications = DeriveModificationsFromXMLTemplate(status.Modifiers);
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.AddModifiers(statusCode, modifications, this.entity);
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
	status.source = { "entity": attacker, "owner": attackerOwner };

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	status._timer = cmpTimer.SetInterval(this.entity, IID_StatusEffectsReceiver, "ExecuteEffect", 0, +(status.Interval || status._interval), statusCode);
};

/**
 * Removes a status effect from the entity.
 *
 * @param {string} statusCode - The status effect to be removed.
 */
StatusEffectsReceiver.prototype.RemoveStatus = function(statusCode)
{
	let statusEffect = this.activeStatusEffects[statusCode];
	if (!statusEffect)
		return;

	if (statusEffect.Modifiers)
	{
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.RemoveAllModifiers(statusCode, this.entity);
	}

	if (statusEffect._timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(statusEffect._timer);
	}
	delete this.activeStatusEffects[statusCode];
};

/**
 * Called by the timers. Executes a status effect.
 *
 * @param {string} statusCode - The status effect to be executed.
 * @param {number} lateness - The delay between the calling of the function and the actual execution (turn time?).
 */
StatusEffectsReceiver.prototype.ExecuteEffect = function(statusCode, lateness)
{
	let status = this.activeStatusEffects[statusCode];
	if (!status)
		return;

	if (status.Damage || status.Capture)
		AttackHelper.HandleAttackEffects(this.entity, {
			"type": statusCode,
			"attackData": status,
			"attacker": status.source.entity,
			"attackerOwner": status.source.owner
		});

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
		this.RemoveStatus(statusCode);
};

Engine.RegisterComponentType(IID_StatusEffectsReceiver, "StatusEffectsReceiver", StatusEffectsReceiver);
