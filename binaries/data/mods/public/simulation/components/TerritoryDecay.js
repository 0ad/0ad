function TerritoryDecay() {}

TerritoryDecay.prototype.Schema =
	"<element name='DecayRate' a:help='Decay rate in capture points per second'>" +
		"<choice><ref name='positiveDecimal'/><value>Infinity</value></choice>" +
	"</element>";

TerritoryDecay.prototype.Init = function()
{
	this.decaying = false;
	this.connectedNeighbours = [];
	this.territoryOwnership = !isFinite(+this.template.DecayRate);
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
		{
			// don't decay if connected to a connected ally; disable blinking
			cmpTerritoryManager.SetTerritoryBlinking(pos.x, pos.y, false);
			return true;
		}

	cmpTerritoryManager.SetTerritoryBlinking(pos.x, pos.y, true);
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

TerritoryDecay.prototype.UpdateOwner = function()
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpOwnership || !cmpPosition || !cmpPosition.IsInWorld())
		return;
	let cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	let pos = cmpPosition.GetPosition2D();
	let tileOwner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
	if (tileOwner != cmpOwnership.GetOwner())
		cmpOwnership.SetOwner(tileOwner);
};

TerritoryDecay.prototype.OnTerritoriesChanged = function(msg)
{
	if (this.territoryOwnership)
		this.UpdateOwner();
	else
		this.UpdateDecayState();
};

TerritoryDecay.prototype.OnTerritoryPositionChanged = function(msg)
{
	if (this.territoryOwnership)
		this.UpdateOwner();
	else
		this.UpdateDecayState();
};

TerritoryDecay.prototype.OnDiplomacyChanged = function(msg)
{
	// Can change the connectedness of certain areas
	if (!this.territoryOwnership)
		this.UpdateDecayState();
};

TerritoryDecay.prototype.OnOwnershipChanged = function(msg)
{
	// Update the list of TerritoryDecay components in the manager
	if (msg.from == -1 || msg.to == -1)
	{
		let cmpTerritoryDecayManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryDecayManager);
		if (msg.from == -1)
			cmpTerritoryDecayManager.Add(this.entity);
		else
			cmpTerritoryDecayManager.Remove(this.entity);
	}

	// if it influences the territory, wait until we get a TerritoriesChanged message
	if (!this.territoryOwnership && !Engine.QueryInterface(this.entity, IID_TerritoryInfluence))
		this.UpdateDecayState();
};

TerritoryDecay.prototype.HasTerritoryOwnership = function()
{
	return this.territoryOwnership;
};

Engine.RegisterComponentType(IID_TerritoryDecay, "TerritoryDecay", TerritoryDecay);
