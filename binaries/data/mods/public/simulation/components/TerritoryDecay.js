function TerritoryDecay() {}

TerritoryDecay.prototype.Schema =
	"<element name='HealthDecayRate' a:help='Decay rate in hitpoints per second'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

TerritoryDecay.prototype.Init = function()
{
	this.timer = undefined;
};

TerritoryDecay.prototype.IsConnected = function()
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return false;

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

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
};

TerritoryDecay.prototype.OnTerritoriesChanged = function(msg)
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

	cmpHealth.Reduce(+this.template.HealthDecayRate);
};

Engine.RegisterComponentType(IID_TerritoryDecay, "TerritoryDecay", TerritoryDecay);
