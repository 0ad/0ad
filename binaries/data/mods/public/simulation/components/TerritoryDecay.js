function TerritoryDecay() {}

TerritoryDecay.prototype.Schema =
	"<element name='DecayRate' a:help='Decay rate in hitpoints per second'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

TerritoryDecay.prototype.Init = function()
{
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

	return cmpTerritoryManager.IsConnected(pos.x, pos.y);
};

TerritoryDecay.prototype.IsDecaying = function()
{
	return this.decaying;
};

TerritoryDecay.prototype.GetDecayRate = function()
{
	return ApplyValueModificationsToEntity(
		"TerritoryDecay/DecayRate",
		+this.template.DecayRate,
		this.entity);
};

TerritoryDecay.prototype.UpdateDecayState = function()
{
	if (this.IsConnected())
		var decaying = false;
	else
		var decaying = this.GetDecayRate() > 0;
	if (decaying === this.decaying)
		return;
	this.decaying = decaying;
	Engine.PostMessage(this.entity, MT_TerritoryDecayChanged, { "entity": this.entity, "to": decaying });
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

Engine.RegisterComponentType(IID_TerritoryDecay, "TerritoryDecay", TerritoryDecay);
