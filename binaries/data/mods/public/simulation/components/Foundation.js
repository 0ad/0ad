function Foundation() {}

Foundation.prototype.Schema =
	"<a:component type='internal'/><empty/>";

Foundation.prototype.Init = function()
{
	// Foundations are initially 'uncommitted' and do not block unit movement at all
	// (to prevent players exploiting free foundations to confuse enemy units).
	// The first builder to reach the uncommitted foundation will tell friendly units
	// and animals to move out of the way, then will commit the foundation and enable
	// its obstruction once there's nothing in the way.
	this.committed = false;

	this.buildProgress = 0.0; // 0 <= progress <= 1
};

Foundation.prototype.InitialiseConstruction = function(owner, template)
{
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);

	this.finalTemplateName = template;
	this.addedHitpoints = cmpHealth.GetHitpoints();
	this.maxHitpoints = cmpHealth.GetMaxHitpoints();

	// We need to know the owner in OnDestroy, but at that point the entity has already been
	// decoupled from its owner, so we need to remember it in here (and assume it won't change)
	this.owner = owner;

	this.initialised = true;
};

Foundation.prototype.GetBuildPercentage = function()
{
	return Math.floor(this.buildProgress * 100);
};

Foundation.prototype.OnDestroy = function()
{
	// Refund a portion of the construction cost, proportional to the amount of build progress remaining

	if (!this.initialised) // this happens if the foundation was destroyed because the player had insufficient resources
		return;

	if (this.buildProgress == 1.0)
		return;

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(this.owner), IID_Player);

	var cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	var costs = cmpCost.GetResourceCosts();
	for (var r in costs)
	{
		var scaled = Math.floor(costs[r] * (1.0 - this.buildProgress));
		if (scaled)
			cmpPlayer.AddResource(r, scaled);
	}
};

/**
 * Perform some number of seconds of construction work.
 * Returns true if the construction is completed.
 */
Foundation.prototype.Build = function(builderEnt, work)
{
	// Do nothing if we've already finished building
	// (The entity will be destroyed soon after completion so
	// this won't happen much)
	if (this.buildProgress == 1.0)
		return;

	// Handle the initial 'committing' of the foundation
	if (!this.committed)
	{
		var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
		if (cmpObstruction)
		{
			// If there's any units in the way, ask them to move away
			// and return early from this method.
			// Otherwise enable this obstruction so it blocks any further
			// units, and continue building.

			var collisions = cmpObstruction.GetConstructionCollisions();
			if (collisions.length)
			{
				for each (var ent in collisions)
				{
					var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
					if (cmpUnitAI)
						cmpUnitAI.LeaveFoundation(this.entity);

					// TODO: What if an obstruction has no UnitAI?
				}

				// TODO: maybe we should tell the builder to use a special
				// animation to indicate they're waiting for people to get
				// out the way

				return;
			}

			// The obstruction always blocks new foundations/construction,
			// but we've temporarily allowed units to walk all over it
			// (via CCmpTemplateManager). Now we need to remove that temporary
			// blocker-disabling, so that we'll perform standard unit blocking instead.
			cmpObstruction.SetDisableBlockMovementPathfinding(false);
		}

		this.committed = true;
	}

	// Calculate the amount of progress that will be added (where 1.0 = completion)
	var cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	var amount = work / cmpCost.GetBuildTime();

	// TODO: implement some kind of diminishing returns for multiple builders.
	// e.g. record the set of entities that build this, then every ~2 seconds
	// count them (and reset the list), and apply some function to the count to get
	// a factor, and apply that factor here.

	this.buildProgress += amount;
	if (this.buildProgress > 1.0)
		this.buildProgress = 1.0;

	// Add an appropriate proportion of hitpoints
	var targetHP = Math.max(0, Math.min(this.maxHitpoints, Math.floor(this.maxHitpoints * this.buildProgress)));
	var deltaHP = targetHP - this.addedHitpoints;
	if (deltaHP > 0)
	{
		var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
		cmpHealth.Increase(deltaHP);
		this.addedHitpoints += deltaHP;
	}

	if (this.buildProgress >= 1.0)
	{
		// Finished construction

		// Create the real entity
		var building = Engine.AddEntity(this.finalTemplateName);

		// Copy various parameters from the foundation

		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		var cmpBuildingPosition = Engine.QueryInterface(building, IID_Position);
		var pos = cmpPosition.GetPosition();
		cmpBuildingPosition.JumpTo(pos.x, pos.z);
		var rot = cmpPosition.GetRotation();
		cmpBuildingPosition.SetYRotation(rot.y);
		cmpBuildingPosition.SetXZRotation(rot.x, rot.z);
		// TODO: should add a ICmpPosition::CopyFrom() instead of all this

		var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		var cmpBuildingOwnership = Engine.QueryInterface(building, IID_Ownership);
		cmpBuildingOwnership.SetOwner(cmpOwnership.GetOwner());
		
		var cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
		cmpPlayerStatisticsTracker.IncreaseConstructedBuildingsCounter();
				
		var cmpIdentity = Engine.QueryInterface(building, IID_Identity);
		if (cmpIdentity.GetClassesList().indexOf("CivCentre") != -1) cmpPlayerStatisticsTracker.IncreaseBuiltCivCentresCounter();
		
		var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
		var cmpBuildingHealth = Engine.QueryInterface(building, IID_Health);
		cmpBuildingHealth.SetHitpoints(cmpHealth.GetHitpoints());

		Engine.PostMessage(this.entity, MT_ConstructionFinished,
			{ "entity": this.entity, "newentity": building });

		Engine.DestroyEntity(this.entity);
	}
};

Engine.RegisterComponentType(IID_Foundation, "Foundation", Foundation);

