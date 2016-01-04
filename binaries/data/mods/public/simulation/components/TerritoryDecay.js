function TerritoryDecay() {}

TerritoryDecay.prototype.Schema =
	"<element name='DecayRate' a:help='Decay rate in capture points per second'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

TerritoryDecay.prototype.Init = function()
{
	this.decaying = false;
	this.connectedNeighbours = [];
};

TerritoryDecay.prototype.IsConnected = function()
{
	var numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	this.connectedNeighbours.fill(0, 0, numPlayers);

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return false;

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return true;// something without ownership can't decay

	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	var pos = cmpPosition.GetPosition2D();
	var tileOwner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
	if (tileOwner == 0)
	{
		this.connectedNeighbours[0] = 1;
		return cmpPlayer.GetPlayerID() == 0; // Gaia building on gaia ground -> don't decay
	}

	var tileConnected = cmpTerritoryManager.IsConnected(pos.x, pos.y);
	if (tileConnected && !cmpPlayer.IsMutualAlly(tileOwner))
	{
		this.connectedNeighbours[tileOwner] = 1;
		return false;
	}

	if (tileConnected)
		return true;

	this.connectedNeighbours = cmpTerritoryManager.GetNeighbours(pos.x, pos.y, true);

	for (var i = 1; i < numPlayers; ++i)
		if (this.connectedNeighbours[i] > 0 && cmpPlayer.IsMutualAlly(i))
			return true; // don't decay if connected to a connected ally

	cmpTerritoryManager.SetTerritoryBlinking(pos.x, pos.y);
	return false;
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

/**
 * Get the number of connected bordering tiles to this region
 * Only valid when this.IsDecaying()
 */
TerritoryDecay.prototype.GetConnectedNeighbours = function()
{
	return this.connectedNeighbours;
};

TerritoryDecay.prototype.UpdateDecayState = function()
{
	let decaying = !this.IsConnected() && this.GetDecayRate() > 0;
	if (decaying === this.decaying)
		return;
	this.decaying = decaying;
	Engine.PostMessage(this.entity, MT_TerritoryDecayChanged, { "entity": this.entity, "to": decaying, "rate": this.GetDecayRate() });
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
	// if it influences the territory, wait until we get a TerritoriesChanged message
	if (!Engine.QueryInterface(this.entity, IID_TerritoryInfluence))
		this.UpdateDecayState();
};

Engine.RegisterComponentType(IID_TerritoryDecay, "TerritoryDecay", TerritoryDecay);
