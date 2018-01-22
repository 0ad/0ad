function Capturable() {}

Capturable.prototype.Schema =
	"<element name='CapturePoints' a:help='Maximum capture points'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='RegenRate' a:help='Number of capture are regenerated per second in favour of the owner'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='GarrisonRegenRate' a:help='Number of capture are regenerated per second and per garrisoned unit in favour of the owner'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Capturable.prototype.Init = function()
{
	// Cache this value
	this.maxCp = +this.template.CapturePoints;
	this.cp = [];
};

//// Interface functions ////

/**
 * Returns the current capture points array
 */
Capturable.prototype.GetCapturePoints = function()
{
	return this.cp;
};

Capturable.prototype.GetMaxCapturePoints = function()
{
	return this.maxCp;
};

Capturable.prototype.GetGarrisonRegenRate = function()
{
	return ApplyValueModificationsToEntity("Capturable/GarrisonRegenRate", +this.template.GarrisonRegenRate, this.entity);
};

/**
 * Set the new capture points, used for cloning entities
 * The caller should assure that the sum of capture points
 * matches the max.
 */
Capturable.prototype.SetCapturePoints = function(capturePointsArray)
{
	this.cp = capturePointsArray;
};

/**
 * Reduces the amount of capture points of an entity,
 * in favour of the player of the source
 * Returns the number of capture points actually taken
 */
Capturable.prototype.Reduce = function(amount, playerID)
{
	if (amount <= 0)
		return 0;

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return 0;

	var cmpPlayerSource = QueryPlayerIDInterface(playerID);
	if (!cmpPlayerSource)
		return 0;

	// Before changing the value, activate Fogging if necessary to hide changes
	var cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	var numberOfEnemies = this.cp.filter((v, i) => v > 0 && cmpPlayerSource.IsEnemy(i)).length;

	if (numberOfEnemies == 0)
		return 0;

	// distribute the capture points over all enemies
	let distributedAmount = amount / numberOfEnemies;
	let removedAmount = 0;
	while (distributedAmount > 0.0001)
	{
		numberOfEnemies = 0;
		for (let i in this.cp)
		{
			if (!this.cp[i] || !cmpPlayerSource.IsEnemy(i))
				continue;
			if (this.cp[i] > distributedAmount)
			{
				removedAmount += distributedAmount;
				this.cp[i] -= distributedAmount;
				++numberOfEnemies;
			}
			else
			{
				removedAmount += this.cp[i];
				this.cp[i] = 0;
			}
		}
		distributedAmount = numberOfEnemies ? (amount - removedAmount) / numberOfEnemies : 0;
	}

	// give all cp taken to the player
	var takenCp = this.maxCp - this.cp.reduce((a, b) => a + b);
	this.cp[playerID] += takenCp;

	this.CheckTimer();
	this.RegisterCapturePointsChanged();
	return takenCp;
};

/**
 * Check if the source can (re)capture points from this building
 */
Capturable.prototype.CanCapture = function(playerID)
{
	var cmpPlayerSource = QueryPlayerIDInterface(playerID);

	if (!cmpPlayerSource)
		warn(playerID + " has no player component defined on its id");
	var cp = this.GetCapturePoints();
	var sourceEnemyCp = 0;
	for (let i in this.GetCapturePoints())
		if (cmpPlayerSource.IsEnemy(i))
			sourceEnemyCp += cp[i];
	return sourceEnemyCp > 0;
};

//// Private functions ////

/**
 * this has to be called whenever the capture points are changed.
 * It notifies other components of the change, and switches ownership when needed
 */
Capturable.prototype.RegisterCapturePointsChanged = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return;

	Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.cp });

	var owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER || this.cp[owner] > 0)
		return;

	// if all cp has been taken from the owner, convert it to the best player
	var bestPlayer = 0;
	for (let i in this.cp)
		if (this.cp[i] >= this.cp[bestPlayer])
			bestPlayer = +i;

	let cmpLostPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpLostPlayerStatisticsTracker)
		cmpLostPlayerStatisticsTracker.LostEntity(this.entity);

	cmpOwnership.SetOwner(bestPlayer);

	let cmpCapturedPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpCapturedPlayerStatisticsTracker)
		cmpCapturedPlayerStatisticsTracker.CapturedEntity(this.entity);
};

Capturable.prototype.GetRegenRate = function()
{
	var regenRate = +this.template.RegenRate;
	regenRate = ApplyValueModificationsToEntity("Capturable/RegenRate", regenRate, this.entity);

	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
		var garrisonRegenRate = this.GetGarrisonRegenRate() * cmpGarrisonHolder.GetEntities().length;
	else
		var garrisonRegenRate = 0;

	return regenRate + garrisonRegenRate;
};

Capturable.prototype.TimerTick = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return;

	var owner = cmpOwnership.GetOwner();
	var modifiedCp = 0;

	// special handle for the territory decay
	// reduce cp from the owner in favour of all neighbours (also allies)
	var cmpTerritoryDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	if (cmpTerritoryDecay && cmpTerritoryDecay.IsDecaying())
	{
		var neighbours = cmpTerritoryDecay.GetConnectedNeighbours();
		var totalNeighbours = neighbours.reduce((a, b) => a + b);
		var decay = Math.min(cmpTerritoryDecay.GetDecayRate(), this.cp[owner]);
		this.cp[owner] -= decay;

		if (totalNeighbours)
			for (let p in neighbours)
				this.cp[p] += decay * neighbours[p] / totalNeighbours;
		else // decay to gaia as default
			this.cp[0] += decay;

		modifiedCp += decay;
		this.RegisterCapturePointsChanged();
	}

	var regenRate = this.GetRegenRate();
	if (regenRate < 0)
		modifiedCp += this.Reduce(-regenRate, 0);
	else if (regenRate > 0)
		modifiedCp += this.Reduce(regenRate, owner);

	if (modifiedCp)
		return;

	// nothing changed, stop the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	this.timer = 0;
	Engine.PostMessage(this.entity, MT_CaptureRegenStateChanged, { "regenerating": false, "regenRate": 0, "territoryDecay": 0 });
};

/**
 * Start the regeneration timer when no timer exists.
 * When nothing can be modified (f.e. because it is fully regenerated), the
 * timer stops automatically after one execution.
 */
Capturable.prototype.CheckTimer = function()
{
	if (this.timer)
		return;

	var regenRate = this.GetRegenRate();
	var cmpDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	var decay = cmpDecay && cmpDecay.IsDecaying() ? cmpDecay.GetDecayRate() : 0;
	if (regenRate == 0 && decay == 0)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Capturable, "TimerTick", 1000, 1000, null);
	Engine.PostMessage(this.entity, MT_CaptureRegenStateChanged, { "regenerating": true, "regenRate": regenRate, "territoryDecay": decay });
};

//// Message Listeners ////

Capturable.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Capturable")
		return;

	var oldMaxCp = this.GetMaxCapturePoints();
	this.maxCp = ApplyValueModificationsToEntity("Capturable/CapturePoints", +this.template.CapturePoints, this.entity);
	if (oldMaxCp == this.maxCp)
		return;

	var scale = this.maxCp / oldMaxCp;
	for (let i in this.cp)
		this.cp[i] *= scale;
	Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.cp });
	this.CheckTimer();
};

Capturable.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	this.CheckTimer();
};

Capturable.prototype.OnTerritoryDecayChanged = function(msg)
{
	if (msg.to)
		this.CheckTimer();
};

Capturable.prototype.OnDiplomacyChanged = function(msg)
{
	this.CheckTimer();
};

Capturable.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == INVALID_PLAYER)
		return; // we're dead

	if (this.cp.length)
	{
		if (!this.cp[msg.from])
			return; // nothing to change

		// Was already initialised, this happens on defeat or wololo
		// transfer the points of the old owner to the new one
		this.cp[msg.to] += this.cp[msg.from];
		this.cp[msg.from] = 0;
		this.RegisterCapturePointsChanged();
	}
	else
	{
		// initialise the capture points when created
		let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		for (let i = 0; i < numPlayers; ++i)
			if (i == msg.to)
				this.cp[i] = this.maxCp;
			else
				this.cp[i] = 0;
	}
	this.CheckTimer();
};

Engine.RegisterComponentType(IID_Capturable, "Capturable", Capturable);
