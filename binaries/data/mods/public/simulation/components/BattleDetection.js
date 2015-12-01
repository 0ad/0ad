function BattleDetection() {}

BattleDetection.prototype.Schema =
	"<a:help>Detects the occurence of battles.</a:help>" +
	"<a:example/>" +
	"<element name='TimerInterval' a:help='Duration of one timer period. Interval over which damage should be recorded in milliseconds'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<element name='RecordLength' a:help='Record length. Number of timer cycles over which damage rate should be calculated'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<element name='DamageRateThreshold' a:help='Damage rate at which alertness is increased'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='AlertnessBattleThreshold' a:help='Alertness at which the player is considered in battle'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='AlertnessPeaceThreshold' a:help='Alertness at which the player is considered at peace'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='AlertnessMax' a:help='Maximum alertness level'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

BattleDetection.prototype.Init = function()
{
	// Load values from template.
	this.interval = +this.template.TimerInterval;
	this.recordLength = +this.template.RecordLength;
	this.damageRateThreshold = +this.template.DamageRateThreshold;
	this.alertnessBattleThreshold = +this.template.AlertnessBattleThreshold;
	this.alertnessPeaceThreshold = +this.template.AlertnessPeaceThreshold;
	this.alertnessMax = +this.template.AlertnessMax;

	// Initialize variables.
	this.damage = 0; // Damage counter. Accumulative damage done over the current timer period.
	this.damageRecord = []; // Damage record. Array of elements representing total damage done in a given timer cycle.
	this.alertness = 0; // Alertness level. Incremented if damage rate exceeds 'damageRateThreshold' over a given timer period and decremented if it does not.
	this.state = "PEACE";
};

BattleDetection.prototype.SetState = function(state)
{
	if (state == this.state)
		return;

	this.state = state;
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	Engine.PostMessage(this.entity, MT_BattleStateChanged, { "player": cmpPlayer.GetPlayerID(), "to": this.state });
};

BattleDetection.prototype.GetState = function()
{
	return this.state;
};

BattleDetection.prototype.TimerHandler = function(data, lateness)
{
	// Reset the timer
	if (data.timerRepeat === undefined)
	{
		this.timer = undefined;
	}

	this.damageRecord.unshift(this.damage);
	if (this.damageRecord.length > this.recordLength)
		this.damageRecord.splice(this.recordLength, this.damageRecord.length-1); // Discard any elements beyond 'recordLength'.
	this.damage = 0; // Reset damage counter for the next timer period.

	// Always update alertness if not already alert, or once per 'recordLength' otherwise.
	if (!this.alertness || this.recordControl++ == this.recordLength-1)
	{
		var recordDamage = this.damageRecord.reduce(function(a, b) {return a + b;}, 0); // Sum up all values in the damage record.
		var damageRate = recordDamage / (this.recordLength * this.interval);

		if (damageRate > this.damageRateThreshold)
			this.alertness = Math.min(this.alertnessMax, this.alertness+1); // Increment alertness up to 'alertnessMax'.
		else
			this.alertness = Math.max(0, this.alertness-1); // Decrement alertness down to zero.

		// Stop the damage rate timer if we're no longer alert.
		if (!this.alertness)
			this.StopTimer();

		if (this.alertness >= this.alertnessBattleThreshold)
			this.SetState("BATTLE");
		else if (this.alertness <= this.alertnessPeaceThreshold)
			this.SetState("PEACE");

	}
	if (this.recordControl > this.recordLength-1)
		this.recordControl = 0;
};

/**
 * Set up the damage rate timer to run after 'offset' msecs, and then optionally
 * every 'repeat' msecs until StopTimer is called, if 'repeat' is set. A "Timer" message
 * will be sent each time the timer runs. Must not be called if a timer is already active.
 */
BattleDetection.prototype.StartTimer = function(offset, repeat)
{
	if (this.timer)
	{
		this.StopTimer();
		error("Called StartTimer when there's already an active timer.");
	}

	this.recordControl = 0;
	this.damage = 0; // Reset damage counter for the first timer period.

	var data = { "timerRepeat": repeat };
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (repeat === undefined)
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BattleDetection, "TimerHandler", offset, data);
	else
		this.timer = cmpTimer.SetInterval(this.entity, IID_BattleDetection, "TimerHandler", offset, repeat, data);
};

/**
 * Stop the current damage rate timer.
 */
BattleDetection.prototype.StopTimer = function()
{
	if (!this.timer)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	this.timer = undefined;
};

BattleDetection.prototype.OnGlobalAttacked = function(msg)
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	// Only register attacks dealt by myself.
	var cmpAttackerOwnership = Engine.QueryInterface(msg.attacker, IID_Ownership);
	if (!cmpAttackerOwnership || cmpAttackerOwnership.GetOwner() != cmpPlayer.GetPlayerID())
		return;
	// Don't register attacks dealt against Gaia or invalid player or myself.
	var cmpTargetOwnership = Engine.QueryInterface(msg.target, IID_Ownership);
	if (!cmpTargetOwnership || cmpTargetOwnership.GetOwner() <= 0 || cmpTargetOwnership.GetOwner() == cmpPlayer.GetPlayerID())
		return;

	// If the damage rate timer isn't already started, start it now.
	if (!this.timer)
		this.StartTimer(0, this.interval);
	// Add damage of this attack to the damage counter.
	if (msg.damage)
		this.damage += msg.damage;
};

Engine.RegisterComponentType(IID_BattleDetection, "BattleDetection", BattleDetection);

