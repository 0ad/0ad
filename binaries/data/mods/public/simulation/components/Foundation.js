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

	// Set up a timer so we can count the number of builders in a 1-second period.
	// (We assume each builder only builds once per second, which is what UnitAI
	// implements.)
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Foundation, "UpdateTimeout", 1000, 1000, {});
	this.recentBuilders = []; // builder entities since the last timeout
	this.numRecentBuilders = 0; // number of builder entities as of the last timeout
};

Foundation.prototype.UpdateTimeout = function()
{
	this.numRecentBuilders = this.recentBuilders.length;
	this.recentBuilders = [];

	Engine.QueryInterface(this.entity, IID_Visual).SetVariable("numbuilders", this.numRecentBuilders);
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

Foundation.prototype.IsFinished = function()
{
	return (this.buildProgress >= 1.0);
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

	// Reset the timer
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
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
		if (cmpObstruction && cmpObstruction.GetBlockMovementFlag())
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
					else
					{
						// If obstructing fauna is gaia but doesn't have UnitAI, just destroy it
						var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
						var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
						if (cmpOwnership && cmpIdentity && cmpOwnership.GetOwner() == 0 && cmpIdentity.HasClass("Animal"))
							Engine.DestroyEntity(ent);
					}

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

	// Record this builder so we can count the total number
	this.recentBuilders.push(builderEnt);

	// TODO: implement some kind of diminishing returns for multiple builders.
	// e.g. record the set of entities that build this, then every ~2 seconds
	// count them (and reset the list), and apply some function to the count to get
	// a factor, and apply that factor here.

	this.buildProgress += amount;
	if (this.buildProgress > 1.0)
		this.buildProgress = 1.0;
		
	Engine.PostMessage(this.entity, MT_FoundationProgressChanged, { "to": this.GetBuildPercentage() });

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

		// ----------------------------------------------------------------------
		
		var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		var cmpBuildingOwnership = Engine.QueryInterface(building, IID_Ownership);
		cmpBuildingOwnership.SetOwner(cmpOwnership.GetOwner());
		
		// ----------------------------------------------------------------------
		
		// Copy over the obstruction control group IDs from the foundation entities. This is needed to ensure that when a foundation
		// is completed and replaced by a new entity, it remains in the same control group(s) as any other foundation entities that 
		// may surround it. This is the mechanism that is used to e.g. enable wall pieces to be built closely together, ignoring their
		// mutual obstruction shapes (since they would otherwise be prevented from being built so closely together). If the control 
		// groups are not copied over, the new entity will default to a new control group containing only itself, and will hence block
		// construction of any surrounding foundations that it was previously in the same control group with.
		
		// Note that this will result in the completed building entities having control group IDs that equal entity IDs of old (and soon
		// to be deleted) foundation entities. This should not have any consequences, however, since the control group IDs are only meant
		// to be unique identifiers, which is still true when reusing the old ones.
		
		var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
		var cmpBuildingObstruction = Engine.QueryInterface(building, IID_Obstruction);
		cmpBuildingObstruction.SetControlGroup(cmpObstruction.GetControlGroup());
		cmpBuildingObstruction.SetControlGroup2(cmpObstruction.GetControlGroup2());
		
		// ----------------------------------------------------------------------
		
		var cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
		cmpPlayerStatisticsTracker.IncreaseConstructedBuildingsCounter();

		var cmpIdentity = Engine.QueryInterface(building, IID_Identity);
		if (cmpIdentity.GetClassesList().indexOf("CivCentre") != -1)
			cmpPlayerStatisticsTracker.IncreaseBuiltCivCentresCounter();
		
		var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
		var cmpBuildingHealth = Engine.QueryInterface(building, IID_Health);
		cmpBuildingHealth.SetHitpoints(cmpHealth.GetHitpoints());

		PlaySound("constructed", building);

		Engine.PostMessage(this.entity, MT_ConstructionFinished,
			{ "entity": this.entity, "newentity": building });
		Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: building });

		Engine.DestroyEntity(this.entity);
	}
};

Engine.RegisterComponentType(IID_Foundation, "Foundation", Foundation);

