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
	this.StartRegenTimer();
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
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == -1)
		return 0;

	var cmpPlayerSource = QueryPlayerIDInterface(playerID);
	if (!cmpPlayerSource)
		return 0;

	// Before changing the value, activate Fogging if necessary to hide changes
	var cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	var enemiesFilter = function(v, i) { return v > 0 && !cmpPlayerSource.IsAlly(i); };
	var numberOfEnemies = this.cp.filter(enemiesFilter).length;

	if (numberOfEnemies == 0)
		return 0;

	// distribute the capture points over all enemies
	var distributedAmount = amount / numberOfEnemies;
	for (let i in this.cp)
	{
		if (cmpPlayerSource.IsAlly(i))
			continue;
		if (this.cp[i] > distributedAmount)
			this.cp[i] -= distributedAmount;
		else
			this.cp[i] = 0;
	}

	// give all cp taken to the player
	var takenCp = this.maxCp - this.cp.reduce(function(a, b) { return a + b; });
	this.cp[playerID] += takenCp;

	this.StartRegenTimer();

	Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.cp })

	if (this.cp[cmpOwnership.GetOwner()] > 0)
		return takenCp;

	// if all cp has been taken from the owner, convert it to the best player
	var bestPlayer = 0;
	for (let i in this.cp)
		if (this.cp[i] >= this.cp[bestPlayer])
			bestPlayer = +i;

	cmpOwnership.SetOwner(bestPlayer);

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
	var cp = this.GetCapturePoints()
	var sourceEnemyCp = 0;
	for (let i in this.GetCapturePoints())
		if (!cmpPlayerSource.IsAlly(i))
			sourceEnemyCp += cp[i];
	return sourceEnemyCp > 0;
};

//// Private functions ////

Capturable.prototype.GetRegenRate = function()
{
	var regenRate = +this.template.RegenRate;
	regenRate = ApplyValueModificationsToEntity("Capturable/RegenRate", regenRate, this.entity);

	var cmpTerritoryDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	if (cmpTerritoryDecay && cmpTerritoryDecay.IsDecaying())
		var territoryDecayRate = cmpTerritoryDecay.GetDecayRate();
	else
		var territoryDecayRate = 0;

	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
		var garrisonRegenRate = this.GetGarrisonRegenRate() * cmpGarrisonHolder.GetEntities().length;
	else
		var garrisonRegenRate = 0;

	return regenRate + garrisonRegenRate - territoryDecayRate;
};

Capturable.prototype.RegenCapturePoints = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == -1)
		return;

	var regenRate = this.GetRegenRate();
	if (regenRate < 0)
		var takenCp = this.Reduce(-regenRate, 0);
	else
		var takenCp = this.Reduce(regenRate, cmpOwnership.GetOwner())

	if (takenCp > 0)
		return;

	// no capture points taken, stop the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.regenTimer);
	this.regenTimer = 0;
	Engine.PostMessage(this.entity, MT_CaptureRegenStateChanged, {"regenerating": false, "rate": 0});
};

/**
 * Start the regeneration timer when no timer exists
 * When nothing can be regenerated (f.e. because the
 * rate is 0, or because it is fully regenerated),
 * the timer stops automatically after one execution.
 */
Capturable.prototype.StartRegenTimer = function()
{
	if (this.regenTimer)
		return;

	var rate = this.GetRegenRate();
	if (rate == 0)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.regenTimer = cmpTimer.SetInterval(this.entity, IID_Capturable, "RegenCapturePoints", 1000, 1000, null);
	Engine.PostMessage(this.entity, MT_CaptureRegenStateChanged, {"regenerating": true, "rate": rate});
};

//// Message Listeners ////

Capturable.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Capturable")
		return;

	var oldMaxCp = this.GetMaxCapturePoints();
	this.maxCp = ApplyValueModificationsToEntity("Capturable/Max", +this.template.Max, this.entity);
	if (oldMaxCp == this.maxCp)
		return;

	var scale = this.maxCp / oldMaxCp;
	for (let i in this.cp)
		this.cp[i] *= scale;
	Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.cp });
	this.StartRegenTimer();
};

Capturable.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	this.StartRegenTimer();
};

Capturable.prototype.OnTerritoryDecayChanged = function(msg)
{
	if (msg.to)
		this.StartRegenTimer();
};

Capturable.prototype.OnOwnershipChanged = function(msg)
{
	this.StartRegenTimer();

	// if the new owner has no capture points, it means that either
	// * it's being initialised now, or
	// * it changed ownership for a different reason (defeat, atlas, ...)
	if (this.cp[msg.to])
		return;

	// initialise the capture points when created
	this.cp = [];
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	for (let i = 0; i < cmpPlayerManager.GetNumPlayers(); ++i)
		if (i == msg.to)
			this.cp[i] = this.maxCp;
		else
			this.cp[i] = 0;
};

Engine.RegisterComponentType(IID_Capturable, "Capturable", Capturable);
