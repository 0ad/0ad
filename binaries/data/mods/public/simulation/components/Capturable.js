function Capturable() {}

Capturable.prototype.Schema =
	"<element name='CapturePoints' a:help='Maximum capture points.'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='RegenRate' a:help='Number of capture points that are regenerated per second in favour of the owner.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='GarrisonRegenRate' a:help='Number of capture points that are regenerated per second and per garrisoned unit in favour of the owner.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Capturable.prototype.Init = function()
{
	this.maxCapturePoints = +this.template.CapturePoints;
	this.garrisonRegenRate = +this.template.GarrisonRegenRate;
	this.regenRate = +this.template.RegenRate;
	this.capturePoints = [];
};

//// Interface functions ////

/**
 * Returns the current capture points array.
 */
Capturable.prototype.GetCapturePoints = function()
{
	return this.capturePoints;
};

Capturable.prototype.GetMaxCapturePoints = function()
{
	return this.maxCapturePoints;
};

Capturable.prototype.GetGarrisonRegenRate = function()
{
	return this.garrisonRegenRate;
};

/**
 * Set the new capture points, used for cloning entities.
 * The caller should assure that the sum of capture points
 * matches the max.
 * @param {number[]} - Array with for all players the new value.
 */
Capturable.prototype.SetCapturePoints = function(capturePointsArray)
{
	this.capturePoints = capturePointsArray;
};

/**
 * Compute the amount of capture points to be reduced and reduce them.
 * @param {number} amount - Number of capture points to be taken.
 * @param {number} captor - The entity capturing us.
 * @param {number} captorOwner - Owner of the captor.
 * @return {Object} - Object of the form { "captureChange": number }, where number indicates the actual amount of capture points taken.
 */
Capturable.prototype.Capture = function(amount, captor, captorOwner)
{
	if (captorOwner == INVALID_PLAYER || !this.CanCapture(captorOwner))
		return {};

	// TODO: implement loot

	return { "captureChange": this.Reduce(amount, captorOwner) };
};

/**
 * Reduces the amount of capture points of an entity,
 * in favour of the player of the source.
 * @param {number} amount - Number of capture points to be taken.
 * @param {number} playerID - ID of player the capture points should be awarded to.
 * @return {number} - The number of capture points actually taken.
 */
Capturable.prototype.Reduce = function(amount, playerID)
{
	if (amount <= 0)
		return 0;

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return 0;

	let cmpPlayerSource = QueryPlayerIDInterface(playerID);
	if (!cmpPlayerSource)
		return 0;

	// Before changing the value, activate Fogging if necessary to hide changes.
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	let numberOfEnemies = this.capturePoints.filter((v, i) => v > 0 && cmpPlayerSource.IsEnemy(i)).length;

	if (numberOfEnemies == 0)
		return 0;

	// Distribute the capture points over all enemies.
	let distributedAmount = amount / numberOfEnemies;
	let removedAmount = 0;
	while (distributedAmount > 0.0001)
	{
		numberOfEnemies = 0;
		for (let i in this.capturePoints)
		{
			if (!this.capturePoints[i] || !cmpPlayerSource.IsEnemy(i))
				continue;
			if (this.capturePoints[i] > distributedAmount)
			{
				removedAmount += distributedAmount;
				this.capturePoints[i] -= distributedAmount;
				++numberOfEnemies;
			}
			else
			{
				removedAmount += this.capturePoints[i];
				this.capturePoints[i] = 0;
			}
		}
		distributedAmount = numberOfEnemies ? (amount - removedAmount) / numberOfEnemies : 0;
	}

	// Give all capture points taken to the player.
	let takenCapturePoints = this.maxCapturePoints - this.capturePoints.reduce((a, b) => a + b);
	this.capturePoints[playerID] += takenCapturePoints;

	this.CheckTimer();
	this.RegisterCapturePointsChanged();
	return takenCapturePoints;
};

/**
 * Check if the source can (re)capture points from this building.
 * @param {number} playerID - PlayerID of the source.
 * @return {boolean} - Whether the source can (re)capture points from this building.
 */
Capturable.prototype.CanCapture = function(playerID)
{
	let cmpPlayerSource = QueryPlayerIDInterface(playerID);

	if (!cmpPlayerSource)
		warn(playerID + " has no player component defined on its id.");
	let capturePoints = this.GetCapturePoints();
	let sourceEnemyCapturePoints = 0;
	for (let i in this.GetCapturePoints())
		if (cmpPlayerSource.IsEnemy(i))
			sourceEnemyCapturePoints += capturePoints[i];
	return sourceEnemyCapturePoints > 0;
};

//// Private functions ////

/**
 * This has to be called whenever the capture points are changed.
 * It notifies other components of the change, and switches ownership when needed.
 */
Capturable.prototype.RegisterCapturePointsChanged = function()
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return;

	Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.capturePoints });

	let owner = cmpOwnership.GetOwner();
	if (owner == INVALID_PLAYER || this.capturePoints[owner] > 0)
		return;

	// If all capture points have been taken from the owner, convert it to player with the most capture points.
	let cmpLostPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpLostPlayerStatisticsTracker)
		cmpLostPlayerStatisticsTracker.LostEntity(this.entity);

	cmpOwnership.SetOwner(this.capturePoints.reduce((bestPlayer, playerCapturePoints, player, capturePoints) => playerCapturePoints > capturePoints[bestPlayer] ? player : bestPlayer, 0));

	let cmpCapturedPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpCapturedPlayerStatisticsTracker)
		cmpCapturedPlayerStatisticsTracker.CapturedEntity(this.entity);
};

Capturable.prototype.GetRegenRate = function()
{
	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (!cmpGarrisonHolder)
		return this.regenRate;

	return this.regenRate + this.GetGarrisonRegenRate() * cmpGarrisonHolder.GetEntities().length;
};

Capturable.prototype.TimerTick = function()
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return;

	let owner = cmpOwnership.GetOwner();
	let modifiedCapturePoints = 0;

	// Special handle for the territory decay.
	// Reduce capture points from the owner in favour of all neighbours (also allies).
	let cmpTerritoryDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	if (cmpTerritoryDecay && cmpTerritoryDecay.IsDecaying())
	{
		let neighbours = cmpTerritoryDecay.GetConnectedNeighbours();
		let totalNeighbours = neighbours.reduce((a, b) => a + b);
		let decay = Math.min(cmpTerritoryDecay.GetDecayRate(), this.capturePoints[owner]);
		this.capturePoints[owner] -= decay;

		if (totalNeighbours)
			for (let p in neighbours)
				this.capturePoints[p] += decay * neighbours[p] / totalNeighbours;
		// Decay to gaia as default.
		else
			this.capturePoints[0] += decay;

		modifiedCapturePoints += decay;
		this.RegisterCapturePointsChanged();
	}

	let regenRate = this.GetRegenRate();
	if (regenRate < 0)
		modifiedCapturePoints += this.Reduce(-regenRate, 0);
	else if (regenRate > 0)
		modifiedCapturePoints += this.Reduce(regenRate, owner);

	if (modifiedCapturePoints)
		return;

	// Nothing changed, stop the timer.
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;
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

	let regenRate = this.GetRegenRate();
	let cmpDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	let decay = cmpDecay && cmpDecay.IsDecaying() ? cmpDecay.GetDecayRate() : 0;
	if (regenRate == 0 && decay == 0)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Capturable, "TimerTick", 1000, 1000, null);
	Engine.PostMessage(this.entity, MT_CaptureRegenStateChanged, { "regenerating": true, "regenRate": regenRate, "territoryDecay": decay });
};

/**
 * Update all chached values that could be affected by modifications.
*/
Capturable.prototype.UpdateCachedValues = function()
{
	this.garrisonRegenRate = ApplyValueModificationsToEntity("Capturable/GarrisonRegenRate", +this.template.GarrisonRegenRate, this.entity);
	this.regenRate = ApplyValueModificationsToEntity("Capturable/RegenRate", +this.template.RegenRate, this.entity);
	this.maxCapturePoints = ApplyValueModificationsToEntity("Capturable/CapturePoints", +this.template.CapturePoints, this.entity);
};

/**
 * Update all chached values that could be affected by modifications.
 * Check timer and send changed messages when required.
 * @param {boolean} message - Whether not to send a CapturePointsChanged message. When false, caller should take care of sending that message.
*/
Capturable.prototype.UpdateCachedValuesAndNotify = function(sendMessage = true)
{
	let oldMaxCapturePoints = this.maxCapturePoints;
	let oldGarrisonRegenRate = this.garrisonRegenRate;
	let oldRegenRate = this.regenRate;

	this.UpdateCachedValues();

	if (oldMaxCapturePoints != this.maxCapturePoints)
	{
		let scale = this.maxCapturePoints / oldMaxCapturePoints;
		for (let i in this.capturePoints)
			this.capturePoints[i] *= scale;
		if (sendMessage)
			Engine.PostMessage(this.entity, MT_CapturePointsChanged, { "capturePoints": this.capturePoints });
	}

	if (oldGarrisonRegenRate != this.garrisonRegenRate || oldRegenRate != this.regenRate)
		this.CheckTimer();
};

//// Message Listeners ////

Capturable.prototype.OnValueModification = function(msg)
{
	if (msg.component == "Capturable")
		this.UpdateCachedValuesAndNotify();
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
		return;

	// Initialise the capture points when created.
	if (!this.capturePoints.length)
	{
		this.UpdateCachedValues();
		let numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
		for (let i = 0; i < numPlayers; ++i)
		{
			if (i == msg.to)
				this.capturePoints[i] = this.maxCapturePoints;
			else
				this.capturePoints[i] = 0;
		}
		this.CheckTimer();
		return;
	}

	// When already initialised, this happens on defeat or wololo,
	// transfer the points of the old owner to the new one.
	if (this.capturePoints[msg.from])
	{
		this.capturePoints[msg.to] += this.capturePoints[msg.from];
		this.capturePoints[msg.from] = 0;
		this.UpdateCachedValuesAndNotify(false);
		this.RegisterCapturePointsChanged();
		return;
	}

	this.UpdateCachedValuesAndNotify();
};

/**
 * When a player is defeated, reassign the capture points of non-owned entities to gaia.
 * Those owned by the defeated player are dealt with onOwnershipChanged.
 */
Capturable.prototype.OnGlobalPlayerDefeated = function(msg)
{
	if (!this.capturePoints[msg.playerId])
		return;
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && (cmpOwnership.GetOwner() == INVALID_PLAYER ||
	                     cmpOwnership.GetOwner() == msg.playerId))
		return;
	this.capturePoints[0] += this.capturePoints[msg.playerId];
	this.capturePoints[msg.playerId] = 0;
	this.RegisterCapturePointsChanged();
	this.CheckTimer();
};

Engine.RegisterComponentType(IID_Capturable, "Capturable", Capturable);
