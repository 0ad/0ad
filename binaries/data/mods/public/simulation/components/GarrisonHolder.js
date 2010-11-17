function GarrisonHolder() {}

GarrisonHolder.prototype.Schema =
	"<element name='Max'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<element name='List'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='EjectHealth'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" + 
	"<element name='BuffHeal'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

/**
 * Initialize GarrisonHolder Component
 */
GarrisonHolder.prototype.Init = function()
{
	//Garrisoned Units
	this.entities = [];
	this.spaceOccupied = 0;
	this.timer = undefined;
	this.healRate = this.template.BuffHeal;
};

/**
 * Return the list of entities garrisoned inside
 */
GarrisonHolder.prototype.GetEntities = function()
{
	return this.entities;
}

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
	return this.template.Max;
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
}
/**
 * Checks if an entity can be allowed to garrison in the building
 * based on it's class
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

	if (!this.timer)
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
		return true;
	}
	else
	{
		return false;
	}
};

/**
 * Unload units from the garrisoning entity
 */
GarrisonHolder.prototype.Unload = function(entity)
{
	var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
	var entityIndex = this.entities.indexOf(entity);
	this.spaceOccupied -= 1;
	this.entities.splice(entityIndex, 1);
	var cmpFootprint = Engine.QueryInterface(this.entity, IID_Footprint);
	var pos = cmpFootprint.PickSpawnPoint(entity);
	if (pos.y < 0)
	{
		// Whoops, something went wrong (maybe there wasn't any space to place the unit).
		// What should we do here?
		// For now, just move the unit into the middle of the building where it'll probably get stuck
		pos = cmpPosition.GetPosition();
		warn("Can't find free space to spawn trained unit");
	}

	var cmpNewPosition = Engine.QueryInterface(entity, IID_Position);
	cmpNewPosition.JumpTo(pos.x, pos.z);
	// TODO: what direction should they face in?

	// If a rally point is set, walk towards it
	var cmpUnitAI = Engine.QueryInterface(entity, IID_UnitAI);
	if (cmpUnitAI && cmpRallyPoint)
	{
		var rallyPos = cmpRallyPoint.GetPosition();
		if (rallyPos)
		{
			cmpUnitAI.Walk(rallyPos.x, rallyPos.z, false);
		}
		else
		{
			//Reset state. This needs to be done since they were walking before being moved
			//out of the world
		}
	}
};

/**
 * Used to check if the garrisoning entity's health has fallen below
 * a certain limit after which all garrisoned units are unloaded
 */
GarrisonHolder.prototype.OnHealthChanged = function(msg)
{
	if (!this.HasEnoughHealth())
	{
		this.UnloadAll();
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
	var ejectHitpoints = parseInt(parseFloat(this.template.EjectHealth) * maxHitpoints);
	return hitpoints > ejectHitpoints; 
};

/**
 * Unload all units from the entity
 */
GarrisonHolder.prototype.UnloadAll = function()
{
	//The entities list is saved to a temporary variable
	//because during each loop an element is removed
	//from the list
	var entities = this.entities.splice(0);
	for each (var entity in entities)
	{
		this.Unload(entity);
	}
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
				if (cmpHealth.GetHitpoints() < cmpHealth.GetMaxHitpoints())
					cmpHealth.Increase(this.healRate);
			}
		}
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_GarrisonHolder, "HealTimeout", 1000, {});
	}
};

GarrisonHolder.prototype.OnOwnershipChanged = function(msg)
{

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

Engine.RegisterComponentType(IID_GarrisonHolder, "GarrisonHolder", GarrisonHolder);
