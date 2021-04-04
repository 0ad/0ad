function GarrisonHolder() {}

GarrisonHolder.prototype.Schema =
	"<element name='Max' a:help='Maximum number of entities which can be garrisoned in this holder'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<element name='List' a:help='Classes of entities which are allowed to garrison in this holder (from Identity)'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='EjectClassesOnDestroy' a:help='Classes of entities to be ejected on destroy. Others are killed'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='BuffHeal' a:help='Number of hitpoints that will be restored to this holder&apos;s garrisoned units each second'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='LoadingRange' a:help='The maximum distance from this holder at which entities are allowed to garrison. Should be about 2.0 for land entities and preferably greater for ships'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='EjectHealth' a:help='Percentage of maximum health below which this holder no longer allows garrisoning'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Pickup' a:help='This garrisonHolder will move to pick up units to be garrisoned'>" +
			"<data type='boolean'/>" +
		"</element>" +
	"</optional>";

/**
 * Time between heals.
 */
GarrisonHolder.prototype.HEAL_TIMEOUT = 1000;

/**
 * Initialize GarrisonHolder Component
 * Garrisoning when loading a map is set in the script of the map, by setting initGarrison
 * which should contain the array of garrisoned entities.
 */
GarrisonHolder.prototype.Init = function()
{
	this.entities = [];
	this.allowedClasses = ApplyValueModificationsToEntity("GarrisonHolder/List/_string", this.template.List._string, this.entity);
};

/**
 * @param {number} entity - The entity to verify.
 * @return {boolean} - Whether the given entity is garrisoned in this GarrisonHolder.
 */
GarrisonHolder.prototype.IsGarrisoned = function(entity)
{
	return this.entities.indexOf(entity) != -1;
};

/**
 * @return {Object} max and min range at which entities can garrison the holder.
 */
GarrisonHolder.prototype.LoadingRange = function()
{
	return { "max": +this.template.LoadingRange, "min": 0 };
};

GarrisonHolder.prototype.CanPickup = function(ent)
{
	if (!this.template.Pickup || this.IsFull())
		return false;
	let cmpOwner = Engine.QueryInterface(this.entity, IID_Ownership);
	return !!cmpOwner && IsOwnedByPlayer(cmpOwner.GetOwner(), ent);
};

GarrisonHolder.prototype.GetEntities = function()
{
	return this.entities;
};

/**
 * @return {Array} unit classes which can be garrisoned inside this
 * particular entity. Obtained from the entity's template.
 */
GarrisonHolder.prototype.GetAllowedClasses = function()
{
	return this.allowedClasses;
};

GarrisonHolder.prototype.GetCapacity = function()
{
	return ApplyValueModificationsToEntity("GarrisonHolder/Max", +this.template.Max, this.entity);
};

GarrisonHolder.prototype.IsFull = function()
{
	return this.OccupiedSlots() >= this.GetCapacity();
};

GarrisonHolder.prototype.GetHealRate = function()
{
	return ApplyValueModificationsToEntity("GarrisonHolder/BuffHeal", +this.template.BuffHeal, this.entity);
};

/**
 * Set this entity to allow or disallow garrisoning in the entity.
 * Every component calling this function should do it with its own ID, and as long as one
 * component doesn't allow this entity to garrison, it can't be garrisoned
 * When this entity already contains garrisoned soldiers,
 * these will not be able to ungarrison until the flag is set to true again.
 *
 * This more useful for modern-day features. For example you can't garrison or ungarrison
 * a driving vehicle or plane.
 * @param {boolean} allow - Whether the entity should be garrisonable.
 */
GarrisonHolder.prototype.AllowGarrisoning = function(allow, callerID)
{
	if (!this.allowGarrisoning)
		this.allowGarrisoning = new Map();
	this.allowGarrisoning.set(callerID, allow);
};

/**
 * @return {boolean} - Whether (un)garrisoning is allowed.
 */
GarrisonHolder.prototype.IsGarrisoningAllowed = function()
{
	return !this.allowGarrisoning ||
		Array.from(this.allowGarrisoning.values()).every(allow => allow);
};

GarrisonHolder.prototype.GetGarrisonedEntitiesCount = function()
{
	let count = this.entities.length;
	for (let ent of this.entities)
	{
		let cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
			count += cmpGarrisonHolder.GetGarrisonedEntitiesCount();
	}
	return count;
};

GarrisonHolder.prototype.OccupiedSlots = function()
{
	let count = 0;
	for (let ent of this.entities)
	{
		let cmpGarrisonable = Engine.QueryInterface(ent, IID_Garrisonable);
		if (cmpGarrisonable)
			count += cmpGarrisonable.TotalSize();
	}
	return count;
};

GarrisonHolder.prototype.IsAllowedToGarrison = function(entity)
{
	if (!this.IsGarrisoningAllowed())
		return false;

	let cmpGarrisonable = Engine.QueryInterface(entity, IID_Garrisonable);
	if (!cmpGarrisonable || this.OccupiedSlots() + cmpGarrisonable.TotalSize() > this.GetCapacity())
		return false;

	return this.IsAllowedToBeGarrisoned(entity);
};

GarrisonHolder.prototype.IsAllowedToBeGarrisoned = function(entity)
{
	if (!IsOwnedByMutualAllyOfEntity(entity, this.entity))
		return false;

	let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
	return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), this.allowedClasses);
};

/**
 * @param {number} entity - The entityID to garrison.
 * @return {boolean} - Whether the entity was garrisoned.
 */
GarrisonHolder.prototype.Garrison = function(entity)
{
	if (!this.IsAllowedToGarrison(entity))
		return false;

	if (!this.HasEnoughHealth())
		return false;

	if (!this.timer && this.GetHealRate())
		this.StartTimer();

	this.entities.push(entity);
	this.UpdateGarrisonFlag();

	Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {
		"added": [entity],
		"removed": []
	});

	return true;
};

/**
 * @param {number} entity - The entity ID of the entity to eject.
 * @param {boolean} forced - Whether eject is forced (e.g. if building is destroyed).
 * @return {boolean} Whether the entity was ejected.
 */
GarrisonHolder.prototype.Eject = function(entity, forced)
{
	if (!this.IsGarrisoningAllowed() && !forced)
		return false;

	let entityIndex = this.entities.indexOf(entity);
	// Error: invalid entity ID, usually it's already been ejected, assume success.
	if (entityIndex == -1)
		return true;

	this.entities.splice(entityIndex, 1);
	this.UpdateGarrisonFlag();
	Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {
		"added": [],
		"removed": [entity]
	});

	return true;
};

/**
 * Tell unit to unload from this entity.
 * @param {number} entity - The entity to unload.
 * @return {boolean} Whether the command was successful.
 */
GarrisonHolder.prototype.Unload = function(entity)
{
	let cmpGarrisonable = Engine.QueryInterface(entity, IID_Garrisonable);
	return cmpGarrisonable && cmpGarrisonable.UnGarrison();
};

/**
 * Tell units to unload from this entity.
 * @param {number[]} entities - The entities to unload.
 * @return {boolean} - Whether all unloads were successful.
 */
GarrisonHolder.prototype.UnloadEntities = function(entities)
{
	let success = true;
	for (let entity of entities)
		if (!this.Unload(entity))
			success = false;
	return success;
};

/**
 * Unload one or all units that match a template and owner from us.
 * @param {string} template - Type of units that should be ejected.
 * @param {number} owner - Id of the player whose units should be ejected.
 * @param {boolean} all - Whether all units should be ejected.
 * @return {boolean} Whether the unloading was successful.
 */
GarrisonHolder.prototype.UnloadTemplate = function(template, owner, all)
{
	let entities = [];
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for (let entity of this.entities)
	{
		let cmpIdentity = Engine.QueryInterface(entity, IID_Identity);

		// Units with multiple ranks are grouped together.
		let name = cmpIdentity.GetSelectionGroupName() || cmpTemplateManager.GetCurrentTemplateName(entity);
		if (name != template || owner != Engine.QueryInterface(entity, IID_Ownership).GetOwner())
			continue;

		entities.push(entity);

		// If 'all' is false, only ungarrison the first matched unit.
		if (!all)
			break;
	}

	return this.UnloadEntities(entities);
};

/**
 * Unload all units, that belong to certain player
 * and order all own units to move to the rally point.
 * @param {number} owner - Id of the player whose units should be ejected.
 * @return {boolean} Whether the unloading was successful.
 */
GarrisonHolder.prototype.UnloadAllByOwner = function(owner)
{
	let entities = this.entities.filter(ent => {
		let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
		return cmpOwnership && cmpOwnership.GetOwner() == owner;
	});
	return this.UnloadEntities(entities);
};

/**
 * Unload all units from the entity and order them to move to the rally point.
 * @return {boolean} Whether the unloading was successful.
 */
GarrisonHolder.prototype.UnloadAll = function()
{
	return this.UnloadEntities(this.entities.slice());
};

/**
 * Used to check if the garrisoning entity's health has fallen below
 * a certain limit after which all garrisoned units are unloaded.
 */
GarrisonHolder.prototype.OnHealthChanged = function(msg)
{
	if (!this.HasEnoughHealth() && this.entities.length)
		this.EjectOrKill(this.entities.slice());
};

GarrisonHolder.prototype.HasEnoughHealth = function()
{
	// 0 is a valid value so explicitly check for undefined.
	if (this.template.EjectHealth === undefined)
		return true;

	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	return !cmpHealth || cmpHealth.GetHitpoints() > Math.floor(+this.template.EjectHealth * cmpHealth.GetMaxHitpoints());
};

GarrisonHolder.prototype.StartTimer = function()
{
	if (this.timer)
		return;
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_GarrisonHolder, "HealTimeout", this.HEAL_TIMEOUT, this.HEAL_TIMEOUT, null);
};

GarrisonHolder.prototype.StopTimer = function()
{
	if (!this.timer)
		return;
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;
};

/**
 * @params data and lateness are unused.
 */
GarrisonHolder.prototype.HealTimeout = function(data, lateness)
{
	let healRate = this.GetHealRate();
	if (!this.entities.length || !healRate)
	{
		this.StopTimer();
		return;
	}

	for (let entity of this.entities)
	{
		let cmpHealth = Engine.QueryInterface(entity, IID_Health);
		if (cmpHealth && !cmpHealth.IsUnhealable())
			cmpHealth.Increase(healRate);
	}
};

/**
 * Updates the garrison flag depending whether something is garrisoned in the entity.
 */
GarrisonHolder.prototype.UpdateGarrisonFlag = function()
{
	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;

	cmpVisual.SetVariant("garrison", this.entities.length ? "garrisoned" : "ungarrisoned");
};

/**
 * Cancel timer when destroyed.
 */
GarrisonHolder.prototype.OnDestroy = function()
{
	if (this.timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};

/**
 * If a garrisoned entity is captured, or about to be killed (so its owner changes to '-1'),
 * remove it from the building so we only ever contain valid entities.
 */
GarrisonHolder.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// The ownership change may be on the garrisonholder
	if (this.entity == msg.entity)
	{
		let entities = this.entities.filter(ent => msg.to == INVALID_PLAYER || !IsOwnedByMutualAllyOfEntity(this.entity, ent));

		if (entities.length)
			this.EjectOrKill(entities);

		return;
	}

	// or on some of its garrisoned units
	let entityIndex = this.entities.indexOf(msg.entity);
	if (entityIndex != -1 && (msg.to == INVALID_PLAYER || !IsOwnedByMutualAllyOfEntity(this.entity, msg.entity)))
		this.EjectOrKill([msg.entity]);
};

/**
 * Update list of garrisoned entities when a game inits.
 */
GarrisonHolder.prototype.OnGlobalSkirmishReplacerReplaced = function(msg)
{
	if (!this.initGarrison)
		return;

	if (msg.entity == this.entity)
	{
		let cmpGarrisonHolder = Engine.QueryInterface(msg.newentity, IID_GarrisonHolder);
		if (cmpGarrisonHolder)
			cmpGarrisonHolder.initGarrison = this.initGarrison;
	}
	else
	{
		let entityIndex = this.initGarrison.indexOf(msg.entity);
		if (entityIndex != -1)
			this.initGarrison[entityIndex] = msg.newentity;
	}
};

/**
 * Eject all foreign garrisoned entities which are no more allied.
 */
GarrisonHolder.prototype.OnDiplomacyChanged = function()
{
	this.EjectOrKill(this.entities.filter(ent => !IsOwnedByMutualAllyOfEntity(this.entity, ent)));
};

/**
 * Eject or kill a garrisoned unit which can no more be garrisoned
 * (garrisonholder's health too small or ownership changed).
 */
GarrisonHolder.prototype.EjectOrKill = function(entities)
{
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	// Eject the units which can be ejected (if not in world, it generally means this holder
	// is inside a holder which kills its entities, so do not eject)
	if (cmpPosition && cmpPosition.IsInWorld())
	{
		let ejectables = entities.filter(ent => this.IsEjectable(ent));
		if (ejectables.length)
			this.UnloadEntities(ejectables);
	}

	// And destroy all remaining entities
	let killedEntities = [];
	for (let entity of entities)
	{
		let entityIndex = this.entities.indexOf(entity);
		if (entityIndex == -1)
			continue;
		let cmpHealth = Engine.QueryInterface(entity, IID_Health);
		if (cmpHealth)
			cmpHealth.Kill();
		else
			Engine.DestroyEntity(entity);
		this.entities.splice(entityIndex, 1);
		killedEntities.push(entity);
	}

	if (killedEntities.length)
	{
		Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {
			"added": [],
			"removed": killedEntities
		});
		this.UpdateGarrisonFlag();
	}
};

/**
 * Whether an entity is ejectable.
 * @param {number} entity - The entity-ID to be tested.
 * @return {boolean} - Whether the entity is ejectable.
 */
GarrisonHolder.prototype.IsEjectable = function(entity)
{
	if (!this.entities.find(ent => ent == entity))
		return false;

	let ejectableClasses = this.template.EjectClassesOnDestroy._string;
	let entityClasses = Engine.QueryInterface(entity, IID_Identity).GetClassesList();

	return MatchesClassList(entityClasses, ejectableClasses);
};

/**
 * Sets the intitGarrison to the specified entities. Used by the mapreader.
 *
 * @param {number[]} entities - The entity IDs to garrison on init.
 */
GarrisonHolder.prototype.SetInitGarrison = function(entities)
{
	this.initGarrison = clone(entities);
};

/**
 * Initialise the garrisoned units.
 */
GarrisonHolder.prototype.OnGlobalInitGame = function(msg)
{
	if (!this.initGarrison)
		return;

	for (let ent of this.initGarrison)
	{
		let cmpGarrisonable = Engine.QueryInterface(ent, IID_Garrisonable);
		if (cmpGarrisonable)
			cmpGarrisonable.Garrison(this.entity);
	}
	delete this.initGarrison;
};

GarrisonHolder.prototype.OnValueModification = function(msg)
{
	if (msg.component != "GarrisonHolder")
		return;

	if (msg.valueNames.indexOf("GarrisonHolder/List/_string") !== -1)
	{
		this.allowedClasses = ApplyValueModificationsToEntity("GarrisonHolder/List/_string", this.template.List._string, this.entity);
		this.EjectOrKill(this.entities.filter(entity => !this.IsAllowedToBeGarrisoned(entity)));
	}

	if (msg.valueNames.indexOf("GarrisonHolder/BuffHeal") === -1)
		return;

	if (this.timer && !this.GetHealRate())
		this.StopTimer();
	else if (!this.timer && this.GetHealRate())
		this.StartTimer();
};

Engine.RegisterComponentType(IID_GarrisonHolder, "GarrisonHolder", GarrisonHolder);
