function TerritoryDecay() {}

TerritoryDecay.prototype.Schema =
	"<element name='HealthDecayRate' a:help='Decay rate in hitpoints per second'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

TerritoryDecay.prototype.Init = function()
{
	this.timer = undefined;
	this.decaying = false;
};

TerritoryDecay.prototype.IsConnected = function()
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return false;

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;
		
	// Prevent special gaia buildings from decaying (e.g. fences, ruins)
	if (cmpOwnership.GetOwner() == 0)
		return true;

	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	if (!cmpTerritoryManager)
		return false;

	var pos = cmpPosition.GetPosition2D();
	var tileOwner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
	if (tileOwner != cmpOwnership.GetOwner())
		return false;
	// TODO: this should probably use the same territory restriction
	// logic as BuildRestrictions, to handle allies etc

	return cmpTerritoryManager.IsConnected(pos.x, pos.y);
};

TerritoryDecay.prototype.IsDecaying = function()
{
	return this.decaying;
};

TerritoryDecay.prototype.UpdateDecayState = function()
{
	var connected = this.IsConnected();
	if (!connected && !this.timer)
	{
		// Start decaying
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetInterval(this.entity, IID_TerritoryDecay, "Decay", 1000, 1000, {});
	}
	else if (connected && this.timer)
	{
		// Stop decaying
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}

	if (connected)
		var decaying = false;
	else
		var decaying = (Math.round(ApplyValueModificationsToEntity("TerritoryDecay/HealthDecayRate", +this.template.HealthDecayRate, this.entity)) > 0);
	if (decaying === this.decaying)
		return;
	this.decaying = decaying;
	Engine.PostMessage(this.entity, MT_TerritoryDecayChanged, { "to": decaying });
};

TerritoryDecay.prototype.OnTerritoriesChanged = function(msg)
{
	this.UpdateDecayState();
};

TerritoryDecay.prototype.OnTerritoryPositionChanged = function(msg)
{
	this.UpdateDecayState();
};

TerritoryDecay.prototype.OnOwnershipChanged = function(msg)
{
	this.UpdateDecayState();
};

TerritoryDecay.prototype.Decay = function()
{
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
		return; // error

	var decayRate = ApplyValueModificationsToEntity("TerritoryDecay/HealthDecayRate", +this.template.HealthDecayRate, this.entity);

	cmpHealth.Reduce(Math.round(decayRate));
};

Engine.RegisterComponentType(IID_TerritoryDecay, "TerritoryDecay", TerritoryDecay);
