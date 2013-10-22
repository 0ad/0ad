function GarrisonHolder() {}

GarrisonHolder.prototype.Schema =
	"<element name='Max' a:help='Maximum number of entities which can be garrisoned inside this holder'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<element name='List' a:help='Classes of entities which are allowed to garrison inside this holder (from Identity)'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='EjectHealth' a:help='Percentage of maximum health below which this holder no longer allows garrisoning'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='EjectEntitiesOnDestroy' a:help='Whether the entity should eject or kill all garrisoned entities on destroy'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='BuffHeal' a:help='Number of hit points that will be restored to this holder&apos;s garrisoned units each second'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='LoadingRange' a:help='The maximum distance from this holder at which entities are allowed to garrison. Should be about 2.0 for land entities and preferably greater for ships'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Initialize GarrisonHolder Component
 */
GarrisonHolder.prototype.Init = function()
{
	// Garrisoned Units
	this.entities = [];
	this.timer = undefined;
	this.allowGarrisoning = {};
};

/**
 * Return range at which entities can garrison here
 */
GarrisonHolder.prototype.GetLoadingRange = function()
{
	var max = +this.template.LoadingRange;
	return { "max": max, "min": 0 };
};

/**
 * Return the list of entities garrisoned inside
 */
GarrisonHolder.prototype.GetEntities = function()
{
	return this.entities;
};

/**
 * Returns an array of unit classes which can be garrisoned inside this
 * particualar entity. Obtained from the entity's template 
 */
GarrisonHolder.prototype.GetAllowedClassesList = function()
{
	var classes = this.template.List._string;
	return classes ? classes.split(/\s+/) : [];
};

/**
 * Get Maximum pop which can be garrisoned
 */
GarrisonHolder.prototype.GetCapacity = function()
{
	return ApplyValueModificationsToEntity("GarrisonHolder/Max", +this.template.Max, this.entity);
};

/**
 * Get the heal rate with which garrisoned units will be healed
 */
GarrisonHolder.prototype.GetHealRate = function()
{
	return ApplyValueModificationsToEntity("GarrisonHolder/BuffHeal", +this.template.BuffHeal, this.entity);
};

GarrisonHolder.prototype.EjectEntitiesOnDestroy = function()
{
	return this.template.EjectEntitiesOnDestroy == "true";
};

/**
 * Set this entity to allow or disallow garrisoning in
 * Every component calling this function should do it with its own ID, and as long as one
 * component doesn't allow this entity to garrison, it can't be garrisoned
 * When this entity already contains garrisoned soldiers, 
 * these will not be able to ungarrison until the flag is set to true again.
 *
 * This more useful for modern-day features. For example you can't garrison or ungarrison
 * a driving vehicle or plane.
 */
GarrisonHolder.prototype.AllowGarrisoning = function(allow, callerID)
{
	this.allowGarrisoning[callerID] = allow;
};

/**
 * Check if no component of this entity blocks garrisoning 
 * (f.e. because the vehicle is moving too fast)
 */
GarrisonHolder.prototype.IsGarrisoningAllowed = function()
{
	for each (var allow in this.allowGarrisoning)
	{
		if (!allow)
			return false;
	}
	return true;
};

/**
 * Get number of garrisoned units capable of shooting arrows
 * Not necessarily archers
 */
GarrisonHolder.prototype.GetGarrisonedArcherCount = function(garrisonArrowClasses)
{
	var count = 0;
	for each (var entity in this.entities)
	{
		var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		var classes = cmpIdentity.GetClassesList();
		if (classes.some(function(c){return garrisonArrowClasses.indexOf(c) > -1;}))
			count++;
	}
	return count;
};

/**
 * Checks if an entity can be allowed to garrison in the building
 * based on its class
 */
GarrisonHolder.prototype.AllowedToGarrison = function(entity)
{
	if (!this.IsGarrisoningAllowed())
		return false;

	var allowedClasses = this.GetAllowedClassesList();
	var entityClasses = (Engine.QueryInterface(entity, IID_Identity)).GetClassesList();
	// Check if the unit is allowed to be garrisoned inside the building
	for each (var allowedClass in allowedClasses)
	{
		if (entityClasses.indexOf(allowedClass) != -1)
		{
			return true;
		}
	}
	return false;
};

/**
 * Garrison a unit inside.
 * Returns true if successful, false if not
 * The timer for AutoHeal is started here
 */
GarrisonHolder.prototype.Garrison = function(entity)
{
	if (!this.HasEnoughHealth())
		return false;

	// Check if the unit is allowed to be garrisoned inside the building
	if(!this.AllowedToGarrison(entity))
		return false;

	if (this.entities.length >= this.GetCapacity())
		return false;

	var cmpPosition = Engine.QueryInterface(entity, IID_Position);
	if (!cmpPosition)
		return false;

	if (!this.timer && this.GetHealRate() > 0)
	{
 		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_GarrisonHolder, "HealTimeout", 1000, {});
	}

	// Actual garrisoning happens here
	this.entities.push(entity);
	cmpPosition.MoveOutOfWorld();
	this.UpdateGarrisonFlag();
	var cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.PauseProduction();

	var cmpAura = Engine.QueryInterface(entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.ApplyGarrisonBonus(this.entity);	

	Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
	return true;
};

/**
 * Simply eject the unit from the garrisoning entity without
 * moving it
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.Eject = function(entity, forced)
{

	var entityIndex = this.entities.indexOf(entity);
	// Error: invalid entity ID, usually it's already been ejected
	if (entityIndex == -1)
		return false; // Fail
	
	// Find spawning location
	var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	var pos = cmpFootprint.PickSpawnPoint(entity);
	if (pos.y < 0)
	{
		// Error: couldn't find space satisfying the unit's passability criteria
		if (forced)
		{	// If ejection is forced, we need to continue, so use center of the building
			var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
			pos = cmpPosition.GetPosition();
		}
		else
		{	// Fail
			return false;
		}
	}
	
	this.entities.splice(entityIndex, 1);
	
	var cmpUnitAI = Engine.QueryInterface(entity, IID_UnitAI);
	if (cmpUnitAI)
		cmpUnitAI.Ungarrison();

	var cmpProductionQueue = Engine.QueryInterface(entity, IID_ProductionQueue);
	if (cmpProductionQueue)
		cmpProductionQueue.UnpauseProduction();

	var cmpAura = Engine.QueryInterface(entity, IID_Auras);
	if (cmpAura && cmpAura.HasGarrisonAura())
		cmpAura.RemoveGarrisonBonus(this.entity);	

	
	var cmpNewPosition = Engine.QueryInterface(entity, IID_Position);
	cmpNewPosition.JumpTo(pos.x, pos.z);
	// TODO: what direction should they face in?
	
	Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
	
	return true;
};

/**
 * Order entities to walk to the Rally Point
 */
GarrisonHolder.prototype.OrderWalkToRallyPoint = function(entities)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	if (cmpRallyPoint)
	{
		var rallyPos = cmpRallyPoint.GetPositions()[0];
		if (rallyPos)
		{
			var commands = GetRallyPointCommands(cmpRallyPoint, entities);
			for each (var com in commands)
			{
				ProcessCommand(cmpOwnership.GetOwner(), com);
			}
		}
	}
};

/**
 * Ejects units and orders them to move to the Rally Point.
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.PerformEject = function(entities, forced)
{
	if (!this.IsGarrisoningAllowed() && !forced)
		return false;

	var ejectedEntities = [];
	var success = true;
	for each (var entity in entities)
	{
		if (this.Eject(entity, forced))
		{
			var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
			var cmpEntOwnership = Engine.QueryInterface(entity, IID_Ownership);
			if (cmpOwnership && cmpEntOwnership && cmpOwnership.GetOwner() == cmpEntOwnership.GetOwner())
				ejectedEntities.push(entity);
		}
		else
			success = false;
	}

	this.OrderWalkToRallyPoint(ejectedEntities);
	this.UpdateGarrisonFlag();

	return success;
};

/**
 * Unload unit from the garrisoning entity and order them
 * to move to the Rally Point
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.Unload = function(entity, forced)
{
	return this.PerformEject([entity], forced);
};

/**
 * Unload one or all units that match a template and owner from
 * the garrisoning entity and order them to move to the Rally Point
 * Returns true if successful, false if not
 * 
 * extendedTemplate has the format "p"+ownerid+"&"+template
 */
GarrisonHolder.prototype.UnloadTemplate = function(extendedTemplate, all, forced)
{
	var index = extendedTemplate.indexOf("&");
	if (index == -1)
		return false;

	var owner = +extendedTemplate.slice(1,index);
	var template = extendedTemplate.slice(index+1);

	var entities = [];
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	for each (var entity in this.entities)
	{
		var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);

		// Units with multiple ranks are grouped together.
		var name = cmpIdentity.GetSelectionGroupName()
		           || cmpTemplateManager.GetCurrentTemplateName(entity);

		if (name != template)
			continue;
		if (owner != Engine.QueryInterface(entity, IID_Ownership).GetOwner())
			continue;

		entities.push(entity);

		// If 'all' is false, only ungarrison the first matched unit.
		if (!all)
			break;
	}

	return this.PerformEject(entities, forced);
};

/**
 * Unload all units with same owner as the entity
 * and order them to move to the Rally Point
 * Returns true if all successful, false if not
 */
GarrisonHolder.prototype.UnloadAllOwn = function(forced)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;
	var owner = cmpOwnership.GetOwner();

	// Make copy of entity list
	var entities = [];
	for each (var entity in this.entities)
	{
		var cmpOwnership = Engine.QueryInterface(entity, IID_Ownership);
		if (cmpOwnership && cmpOwnership.GetOwner() == owner)
			entities.push(entity);
	}
	
	return this.PerformEject(entities, forced);
};

/**
 * Unload all units from the entity
 * and order them to move to the Rally Point
 * Returns true if all successful, false if not
 */
GarrisonHolder.prototype.UnloadAll = function(forced)
{
	var entities = this.entities.slice(0);
	return this.PerformEject(entities, forced);
};

/**
 * Used to check if the garrisoning entity's health has fallen below
 * a certain limit after which all garrisoned units are unloaded
 */
GarrisonHolder.prototype.OnHealthChanged = function(msg)
{
	if (!this.HasEnoughHealth())
		this.EjectOrKill(this.entities);
};

/**
 * Check if this entity has enough health to garrison units inside it
 */
GarrisonHolder.prototype.HasEnoughHealth = function()
{
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health)
	var hitpoints = cmpHealth.GetHitpoints();
	var maxHitpoints = cmpHealth.GetMaxHitpoints();
	var ejectHitpoints = Math.floor((+this.template.EjectHealth) * maxHitpoints);
	return hitpoints > ejectHitpoints;
};

/**
 * Called every second. Heals garrisoned units
 */
GarrisonHolder.prototype.HealTimeout = function(data)
{
	if (this.entities.length == 0)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}
	else
	{
		for each (var entity in this.entities)
		{
			var cmpHealth = Engine.QueryInterface(entity, IID_Health);
			if (cmpHealth)
			{
				// We do not want to heal unhealable units
				if (!cmpHealth.IsUnhealable())
					cmpHealth.Increase(this.GetHealRate());
			}
		}
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_GarrisonHolder, "HealTimeout", 1000, {});
	}
};

GarrisonHolder.prototype.UpdateGarrisonFlag = function()
{
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpVisual)
		return;
	cmpVisual.SelectAnimation("garrisoned", true, 0, "");
	// TODO: ought to extend ICmpVisual to let us just select variant
	// keywords without changing the animation too
	if (this.entities.length)
		cmpVisual.SelectAnimation("garrisoned", false, 1.0, "");
	else
		cmpVisual.SelectAnimation("idle", false, 1.0, "");
};

/**
 * Cancel timer when destroyed
 */
GarrisonHolder.prototype.OnDestroy = function()
{
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
	}
};

/**
 * If a garrisoned entity is captured, or about to be killed (so its owner
 * changes to '-1'), remove it from the building so we only ever contain valid
 * entities
 */
GarrisonHolder.prototype.OnGlobalOwnershipChanged = function(msg)
{
	// the ownership change may be on the garrisonholder
	if (this.entity == msg.entity)
	{
		var entities = [];
		for each (var entity in this.entities)
		{
			if (msg.to == -1 || !IsOwnedByMutualAllyOfEntity(this.entity, entity))
				entities.push(entity);
		}
		this.EjectOrKill(entities);
		return;
	}

	// or on some of its garrisoned units
	var entityIndex = this.entities.indexOf(msg.entity);
	if (entityIndex != -1)
	{
		// If the entity is dead, remove it directly instead of ejecting the corpse
		var cmpHealth = Engine.QueryInterface(msg.entity, IID_Health);
		if (cmpHealth && cmpHealth.GetHitpoints() == 0)
		{
			this.entities.splice(entityIndex, 1);
			Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
			this.UpdateGarrisonFlag();
		}
		else if(!IsOwnedByMutualAllyOfEntity(this.entity, this.entities[entityIndex]))
			this.EjectOrKill([this.entities[entityIndex]]);
	}
};

/**
 * Update list of garrisoned entities if one gets renamed (e.g. by promotion)
 */
GarrisonHolder.prototype.OnGlobalEntityRenamed = function(msg)
{
	var entityIndex = this.entities.indexOf(msg.entity);
	if (entityIndex != -1)
	{
		this.entities[entityIndex] = msg.newentity;
		Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
	}
};


/**
 * Eject all foreign garrisoned entities which are no more allied
 */
GarrisonHolder.prototype.OnDiplomacyChanged = function()
{
	var entities = []
	for each (var entity in this.entities)
	{
		if (!IsOwnedByMutualAllyOfEntity(this.entity, entity))
			entities.push(entity);
	}
	this.EjectOrKill(entities);
};

/**
 * Eject or kill a garrisoned unit which can no more be garrisoned
 * (garrisonholder's health too small or ownership changed)
 */
GarrisonHolder.prototype.EjectOrKill = function(entities)
{
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	// Destroy the garrisoned units if the holder kill his entities on destroy or
	// is not in the world (generally means this holder is inside 
	// a holder which kills its entities which has sunk).
	if (!this.EjectEntitiesOnDestroy() || !cmpPosition.IsInWorld())
	{
		for each (var entity in entities)
		{
			var cmpHealth = Engine.QueryInterface(entity, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			var entityIndex = this.entities.indexOf(entity);
			this.entities.splice(entityIndex, 1);
			Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
		}
		this.UpdateGarrisonFlag();
	}
	else
	{	// Building - force ejection
		this.PerformEject(entities, true);
	}
};

Engine.RegisterComponentType(IID_GarrisonHolder, "GarrisonHolder", GarrisonHolder);

