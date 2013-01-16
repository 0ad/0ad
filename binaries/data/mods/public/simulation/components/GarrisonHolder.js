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
	this.spaceOccupied = 0;
	this.timer = undefined;
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
	var string = this.template.List._string;
	return string.split(/\s+/);
};

/**
 * Get Maximum pop which can be garrisoned
 */
GarrisonHolder.prototype.GetCapacity = function()
{
	return ApplyTechModificationsToEntity("GarrisonHolder/Max", +this.template.Max, this.entity);
};

/**
 * Get the heal rate with which garrisoned units will be healed
 */
GarrisonHolder.prototype.GetHealRate = function()
{
	return ApplyTechModificationsToEntity("GarrisonHolder/BuffHeal", +this.template.BuffHeal, this.entity);
};

/**
 * Get number of garrisoned units capable of shooting arrows
 * Not necessarily archers
 */
GarrisonHolder.prototype.GetGarrisonedArcherCount = function()
{
	var count = 0;
	for each (var entity in this.entities)
	{
		var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
		var classes = cmpIdentity.GetClassesList();
		if (classes.indexOf("Infantry") != -1 || classes.indexOf("Ranged") != -1)
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
	var entityPopCost = (Engine.QueryInterface(entity, IID_Cost)).GetPopCost();
	var entityClasses = (Engine.QueryInterface(entity, IID_Identity)).GetClassesList();

	if (!this.HasEnoughHealth())
		return false;

	// Check if the unit is allowed to be garrisoned inside the building
	if(!this.AllowedToGarrison(entity))
		return false;

	if (this.GetCapacity() < this.spaceOccupied + 1)
		return false;

	if (!this.timer && this.GetHealRate() > 0)
	{
 		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_GarrisonHolder, "HealTimeout", 1000, {});
	}

	var cmpPosition = Engine.QueryInterface(entity, IID_Position);
	if (cmpPosition)
	{
		// Actual garrisoning happens here
		this.entities.push(entity);
		this.spaceOccupied += 1;
		cmpPosition.MoveOutOfWorld();
		this.UpdateGarrisonFlag();
		Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
		return true;
	}
	return false;
};

/**
 * Simply eject the unit from the garrisoning entity without
 * moving it
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.Eject = function(entity, forced)
{
	var entityIndex = this.entities.indexOf(entity);
	if (entityIndex == -1)
	{	// Error: invalid entity ID, usually it's already been ejected
		return false; // Fail
	}
	
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
	
	this.spaceOccupied -= 1;
	this.entities.splice(entityIndex, 1);
	
	var cmpUnitAI = Engine.QueryInterface(entity, IID_UnitAI);
	if (cmpUnitAI)
	{
		cmpUnitAI.Ungarrison();
	}
	
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
 * Unload units from the garrisoning entity and order them
 * to move to the Rally Point
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.Unload = function(entity, forced)
{
	if (this.Eject(entity, forced))
	{
		this.OrderWalkToRallyPoint([entity]);
		this.UpdateGarrisonFlag();
		return true;
	}
	
	return false;
};

/**
 * Unload one or all units that match a template from the
 * garrisoning entity and order them to move to the Rally Point
 * Returns true if successful, false if not
 */
GarrisonHolder.prototype.UnloadTemplate = function(template, all, forced)
{
	var ejectedEntities = [];
	var success = true;
	for (var i = 0; i < this.entities.length; ++i)
	{
		var entity = this.entities[i];
		var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);

		// Units with multiple ranks are grouped together.
		var name = cmpIdentity.GetSelectionGroupName();
		if (!name)
		{
			var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
			name = cmpTemplateManager.GetCurrentTemplateName(entity);
		}
		
		if (name != template)
			continue;

		if (this.Eject(entity, forced))
		{
			--i; // Decrement 'i' as Eject() shortens the array.
			ejectedEntities.push(entity);

			// If 'all' is false, only ungarrison the first matched unit.
			if (!all)
				break;
		}
		else
			success = false;
	}

	this.OrderWalkToRallyPoint(ejectedEntities);
	this.UpdateGarrisonFlag();

	return success;
};

/**
 * Unload all units from the entity
 * Returns true if all successful, false if not
 */
GarrisonHolder.prototype.UnloadAll = function(forced)
{
	// Make copy of entity list
	var entities = [];
	for each (var entity in this.entities)
	{
		entities.push(entity);
	}
	
	var ejectedEntities = [];
	var success = true;
	for each (var entity in entities)
	{
		if (this.Eject(entity, forced))
		{
			ejectedEntities.push(entity);
		}
		else
		{
			success = false;
		}
	}
	
	this.OrderWalkToRallyPoint(ejectedEntities);
	this.UpdateGarrisonFlag();
	
	return success;
};

/**
 * Used to check if the garrisoning entity's health has fallen below
 * a certain limit after which all garrisoned units are unloaded
 */
GarrisonHolder.prototype.OnHealthChanged = function(msg)
{
	if (!this.HasEnoughHealth())
	{
		// We have to be careful of our passability
		//	ships: not land passable, so assume units have drowned in a shipwreck
		//  building: land passable, so units can be ejected freely
		var classes = (Engine.QueryInterface(this.entity, IID_Identity)).GetClassesList();
		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);

		// Destroy the garrisoned units if the holder is a ship or is not in the
		// world (generally means this holder is inside a ship which has sunk).
		if (classes.indexOf("Ship") != -1 || !cmpPosition.IsInWorld())
		{
			for each (var entity in this.entities)
			{
				var cmpHealth = Engine.QueryInterface(entity, IID_Health);
				if (cmpHealth)
				{
					cmpHealth.Kill();
				}
			}
			this.entities = [];
			Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
		}
		else
		{	// Building - force ejection
			this.UnloadAll(true);
		}
	}
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
	var entityIndex = this.entities.indexOf(msg.entity);
	if (entityIndex != -1)
	{
		// If the entity is dead, remove it directly instead of ejecting the corpse
		var cmpHealth = Engine.QueryInterface(msg.entity, IID_Health);
		if (cmpHealth && cmpHealth.GetHitpoints() == 0)
		{
			this.entities.splice(entityIndex, 1);
			Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
		}
		else
		{
			// We have to be careful of our passability
			//	ships: not land passable, assume unit was thrown overboard or something
			//  building: land passable, unit can be ejected freely
			var classes = (Engine.QueryInterface(this.entity, IID_Identity)).GetClassesList();
			if (classes.indexOf("Ship") != -1)
			{	// Ship - kill unit
				var cmpHealth = Engine.QueryInterface(msg.entity, IID_Health);
				if (cmpHealth)
				{
					cmpHealth.Kill();
				}
				this.entities.splice(entityIndex, 1);
				Engine.PostMessage(this.entity, MT_GarrisonedUnitsChanged, {});
			}
			else
			{	// Building - force ejection
				this.Eject(msg.entity, true);
			}
		}
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

Engine.RegisterComponentType(IID_GarrisonHolder, "GarrisonHolder", GarrisonHolder);

