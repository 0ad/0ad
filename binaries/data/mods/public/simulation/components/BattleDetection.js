function BattleDetection() {}

BattleDetection.prototype.Schema =
	"<a:help>Detects the occurence of battles.</a:help>" +
	"<a:example/>" +
	"<a:component type='system'/>" +
	"<empty/>";

BattleDetection.prototype.Init = function()
{
	this.interval = 5 * 1000; // Duration of one timer period. Interval over which damage rate should be calculated in milliseconds.
	this.damageRateThreshold = 0.04; // Damage rate at which alertness is increased.
	this.alertnessBattleThreshold = 2; // Alertness at which the player is considered in battle.
	this.alertnessPeaceThreshold = 0; // Alertness at which the player is considered at peace.
	this.alertnessMax = 4; // Maximum alertness level.

	this.damage = 0; // Accumulative damage dealt over the current timer period.
	this.damageRate = 0; // Damage rate. Total damage dealt over the previous timer period.
	this.alertness = 0; // Alertness level. Incremented if damage rate exceeds 'damageRateThreshold' over a given timer period and decremented if it does not.

	this.StartTimer(0, this.interval);
};

BattleDetection.prototype.setState = function(state)
{
	if (state != this.state) {
		var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
		this.state = state;
		Engine.PostMessage(this.entity, MT_BattleStateChanged, { "player": cmpPlayer.GetPlayerID(), "to": this.state });
	}
};

BattleDetection.prototype.updateAlertness = function()
{
	if (this.damageRate > this.damageRateThreshold)
		this.alertness = Math.min(this.alertnessMax, this.alertness+1); // Increment alertness up to 'alertnessMax'.
	else
		this.alertness = Math.max(0, this.alertness-1); // Decrement alertness down to zero.

	if (this.alertness >= this.alertnessBattleThreshold)
		this.setState("BATTLE");
	else if (this.alertness <= this.alertnessPeaceThreshold)
		this.setState("PEACE");
}

BattleDetection.prototype.GetState = function()
{
	return this.state;
}

BattleDetection.prototype.TimerHandler = function(data, lateness)
{
	// Reset the timer
	if (data.timerRepeat === undefined)
	{
		this.timer = undefined;
	}

	this.damageRate = this.damage / this.interval; // Define damage rate as total damage dealt per unit 'interval' (i.e. millisecond) over the previous timer period.
	this.damage = 0; // Reset damage counter for the next timer period.
	this.updateAlertness();
};

/**
 * Set up the BattleDetection timer to run after 'offset' msecs, and then optionally
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

	var data = { "timerRepeat": repeat };

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (repeat === undefined)
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BattleDetection, "TimerHandler", offset, data);
	else
		this.timer = cmpTimer.SetInterval(this.entity, IID_BattleDetection, "TimerHandler", offset, repeat, data);
};

/**
 * Stop the current BattleDetection timer.
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
	// Don't register attacks dealt against Gaia or invalid player.	
	var cmpTargetOwnership = Engine.QueryInterface(msg.target, IID_Ownership);
	if (!cmpTargetOwnership || cmpTargetOwnership.GetOwner() <= 0)
		return;

	if (msg.damage)
		this.damage += msg.damage;
};

Engine.RegisterComponentType(IID_BattleDetection, "BattleDetection", BattleDetection);
