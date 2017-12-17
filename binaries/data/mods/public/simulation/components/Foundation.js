function Foundation() {}

Foundation.prototype.Schema =
	"<empty/>";

Foundation.prototype.Init = function()
{
	// Foundations are initially 'uncommitted' and do not block unit movement at all
	// (to prevent players exploiting free foundations to confuse enemy units).
	// The first builder to reach the uncommitted foundation will tell friendly units
	// and animals to move out of the way, then will commit the foundation and enable
	// its obstruction once there's nothing in the way.
	this.committed = false;

	this.builders = new Map(); // Map of builder entities to their work per second
	this.totalBuilderRate = 0; // Total amount of work the builders do each second
	this.buildMultiplier = 1; // Multiplier for the amount of work builders do
	this.buildTimePenalty = 0.7; // Penalty for having multiple builders

	this.previewEntity = INVALID_ENTITY;
};

Foundation.prototype.InitialiseConstruction = function(owner, template)
{
	this.finalTemplateName = template;

	// We need to know the owner in OnDestroy, but at that point the entity has already been
	// decoupled from its owner, so we need to remember it in here (and assume it won't change)
	this.owner = owner;

	// Remember the cost here, so if it changes after construction begins (from auras or technologies)
	// we will use the correct values to refund partial construction costs
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	if (!cmpCost)
		error("A foundation must have a cost component to know the build time");

	this.costs = cmpCost.GetResourceCosts(owner);

	this.maxProgress = 0;

	this.initialised = true;
};

/**
 * Moving the revelation logic from Build to here makes the building sink if
 * it is attacked.
 */
Foundation.prototype.OnHealthChanged = function(msg)
{
	// Gradually reveal the final building preview
	var cmpPosition = Engine.QueryInterface(this.previewEntity, IID_Position);
	if (cmpPosition)
		cmpPosition.SetConstructionProgress(this.GetBuildProgress());

	Engine.PostMessage(this.entity, MT_FoundationProgressChanged, { "to": this.GetBuildPercentage() });
};

/**
 * Returns the current build progress in a [0,1] range.
 */
Foundation.prototype.GetBuildProgress = function()
{
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
		return 0;

	var hitpoints = cmpHealth.GetHitpoints();
	var maxHitpoints = cmpHealth.GetMaxHitpoints();

	return hitpoints / maxHitpoints;
};

Foundation.prototype.GetBuildPercentage = function()
{
	return Math.floor(this.GetBuildProgress() * 100);
};

Foundation.prototype.GetNumBuilders = function()
{
	return this.builders.size;
};

Foundation.prototype.IsFinished = function()
{
	return (this.GetBuildProgress() == 1.0);
};

Foundation.prototype.OnDestroy = function()
{
	// Refund a portion of the construction cost, proportional to the amount of build progress remaining

	if (!this.initialised) // this happens if the foundation was destroyed because the player had insufficient resources
		return;

	if (this.previewEntity != INVALID_ENTITY)
	{
		Engine.DestroyEntity(this.previewEntity);
		this.previewEntity = INVALID_ENTITY;
	}

	if (this.IsFinished())
		return;

	let cmpPlayer = QueryPlayerIDInterface(this.owner);

	for (var r in this.costs)
	{
		var scaled = Math.ceil(this.costs[r] * (1.0 - this.maxProgress));
		if (scaled)
		{
			cmpPlayer.AddResource(r, scaled);
			var cmpStatisticsTracker = QueryPlayerIDInterface(this.owner, IID_StatisticsTracker);
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseResourceUsedCounter(r, -scaled);
		}
	}
};

/**
 * Adds a builder to the counter.
 */
Foundation.prototype.AddBuilder = function(builderEnt)
{
	if (this.builders.has(builderEnt))
		return;

	this.builders.set(builderEnt, Engine.QueryInterface(builderEnt, IID_Builder).GetRate());
	this.totalBuilderRate += this.builders.get(builderEnt);
	this.SetBuildMultiplier();

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SetVariable("numbuilders", this.builders.size);

	Engine.PostMessage(this.entity, MT_FoundationBuildersChanged, { "to": Array.from(this.builders.keys()) });
};

Foundation.prototype.RemoveBuilder = function(builderEnt)
{
	if (!this.builders.has(builderEnt))
		return;

	this.totalBuilderRate -= this.builders.get(builderEnt);
	this.builders.delete(builderEnt);
	this.SetBuildMultiplier();

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SetVariable("numbuilders", this.builders.size);

	Engine.PostMessage(this.entity, MT_FoundationBuildersChanged, { "to": Array.from(this.builders.keys()) });
};

/**
 * The build multiplier is a penalty that is applied to each builder.
 * For example, ten women build at a combined rate of 10^0.7 = 5.01 instead of 10.
 */
Foundation.prototype.CalculateBuildMultiplier = function(num)
{
	return num < 2 ? 1 : Math.pow(num, this.buildTimePenalty) / num;
};

Foundation.prototype.SetBuildMultiplier = function()
{
	this.buildMultiplier = this.CalculateBuildMultiplier(this.GetNumBuilders());
};

Foundation.prototype.GetBuildTime = function()
{
	let timeLeft = (1 - this.GetBuildProgress()) * Engine.QueryInterface(this.entity, IID_Cost).GetBuildTime();
	let rate = this.totalBuilderRate * this.buildMultiplier;
	// The rate if we add another woman to the foundation.
	let rateNew = (this.totalBuilderRate + 1) * this.CalculateBuildMultiplier(this.GetNumBuilders() + 1);
	return {
		"timeRemaining": timeLeft / rate,
		"timeSpeedup": timeLeft / rate - timeLeft / rateNew
	};
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
	if (this.GetBuildProgress() == 1.0)
		return;

	// If there's any units in the way, ask them to move away
	// and return early from this method.
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction && cmpObstruction.GetBlockMovementFlag())
	{
		var collisions = cmpObstruction.GetUnitCollisions();
		if (collisions.length)
		{
			var cmpFoundationOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
			for (var ent of collisions)
			{
				var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
				if (cmpUnitAI)
					cmpUnitAI.LeaveFoundation(this.entity);
				else
				{
					// If obstructing fauna is gaia or our own but doesn't have UnitAI, just destroy it
					var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
					var cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
					if (cmpOwnership && cmpIdentity && cmpIdentity.HasClass("Animal")
						&& (cmpOwnership.GetOwner() == 0 || cmpFoundationOwnership && cmpOwnership.GetOwner() == cmpFoundationOwnership.GetOwner()))
						Engine.DestroyEntity(ent);
				}

				// TODO: What if an obstruction has no UnitAI?
			}

			// TODO: maybe we should tell the builder to use a special
			// animation to indicate they're waiting for people to get
			// out the way

			return;
		}
	}

	// Handle the initial 'committing' of the foundation
	if (!this.committed)
	{
		// The obstruction always blocks new foundations/construction,
		// but we've temporarily allowed units to walk all over it
		// (via CCmpTemplateManager). Now we need to remove that temporary
		// blocker-disabling, so that we'll perform standard unit blocking instead.
		if (cmpObstruction && cmpObstruction.GetBlockMovementFlag())
			cmpObstruction.SetDisableBlockMovementPathfinding(false, false, -1);

		// Call the related trigger event
		var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
		cmpTrigger.CallEvent("ConstructionStarted", {
			"foundation": this.entity,
			"template": this.finalTemplateName
		});

		// Switch foundation to scaffold variant
		var cmpFoundationVisual = Engine.QueryInterface(this.entity, IID_Visual);
		if (cmpFoundationVisual)
			cmpFoundationVisual.SelectAnimation("scaffold", false, 1.0, "");

		// Create preview entity and copy various parameters from the foundation
		if (cmpFoundationVisual && cmpFoundationVisual.HasConstructionPreview())
		{
			this.previewEntity = Engine.AddEntity("construction|"+this.finalTemplateName);
			var cmpFoundationOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
			var cmpPreviewOwnership = Engine.QueryInterface(this.previewEntity, IID_Ownership);
			cmpPreviewOwnership.SetOwner(cmpFoundationOwnership.GetOwner());

			// Initially hide the preview underground
			var cmpPreviewPosition = Engine.QueryInterface(this.previewEntity, IID_Position);
			cmpPreviewPosition.SetConstructionProgress(0.0);

			var cmpPreviewVisual = Engine.QueryInterface(this.previewEntity, IID_Visual);
			if (cmpPreviewVisual)
			{
				cmpPreviewVisual.SetActorSeed(cmpFoundationVisual.GetActorSeed());
				cmpPreviewVisual.SelectAnimation("scaffold", false, 1.0, "");
			}

			var cmpFoundationPosition = Engine.QueryInterface(this.entity, IID_Position);
			var pos = cmpFoundationPosition.GetPosition2D();
			var rot = cmpFoundationPosition.GetRotation();
			cmpPreviewPosition.SetYRotation(rot.y);
			cmpPreviewPosition.SetXZRotation(rot.x, rot.z);
			cmpPreviewPosition.JumpTo(pos.x, pos.y);
		}

		this.committed = true;
	}

	// Add an appropriate proportion of hitpoints
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
	{
		error("Foundation " + this.entity + " does not have a health component.");
		return;
	}
	var deltaHP = work * this.GetBuildRate() * this.buildMultiplier;
	if (deltaHP > 0)
		cmpHealth.Increase(deltaHP);

	// Update the total builder rate
	this.totalBuilderRate += work - this.builders.get(builderEnt);
	this.builders.set(builderEnt, work);

	var progress = this.GetBuildProgress();

	// Remember our max progress for partial refund in case of destruction
	this.maxProgress = Math.max(this.maxProgress, progress);

	if (progress >= 1.0)
	{
		// Finished construction

		// Create the real entity
		var building = Engine.AddEntity(this.finalTemplateName);

		// Copy various parameters from the foundation

		var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
		var cmpBuildingVisual = Engine.QueryInterface(building, IID_Visual);
		if (cmpVisual && cmpBuildingVisual)
			cmpBuildingVisual.SetActorSeed(cmpVisual.GetActorSeed());

		var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
		if (!cmpPosition || !cmpPosition.IsInWorld())
		{
			error("Foundation " + this.entity + " does not have a position in-world.");
			Engine.DestroyEntity(building);
			return;
		}
		var cmpBuildingPosition = Engine.QueryInterface(building, IID_Position);
		if (!cmpBuildingPosition)
		{
			error("New building " + building + " has no position component.");
			Engine.DestroyEntity(building);
			return;
		}
		var pos = cmpPosition.GetPosition2D();
		cmpBuildingPosition.JumpTo(pos.x, pos.y);
		var rot = cmpPosition.GetRotation();
		cmpBuildingPosition.SetYRotation(rot.y);
		cmpBuildingPosition.SetXZRotation(rot.x, rot.z);
		// TODO: should add a ICmpPosition::CopyFrom() instead of all this

		var cmpRallyPoint = Engine.QueryInterface(this.entity, IID_RallyPoint);
		var cmpBuildingRallyPoint = Engine.QueryInterface(building, IID_RallyPoint);
		if(cmpRallyPoint && cmpBuildingRallyPoint)
		{
			var rallyCoords = cmpRallyPoint.GetPositions();
			var rallyData = cmpRallyPoint.GetData();
			for (var i = 0; i < rallyCoords.length; ++i)
			{
				cmpBuildingRallyPoint.AddPosition(rallyCoords[i].x, rallyCoords[i].z);
				cmpBuildingRallyPoint.AddData(rallyData[i]);
			}
		}

		// ----------------------------------------------------------------------

		var owner;
		var cmpTerritoryDecay = Engine.QueryInterface(building, IID_TerritoryDecay);
		if (cmpTerritoryDecay && cmpTerritoryDecay.HasTerritoryOwnership())
		{
			let cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
			owner = cmpTerritoryManager.GetOwner(pos.x, pos.y);
		}
		else
		{
			let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
			if (!cmpOwnership)
			{
				error("Foundation " + this.entity + " has no ownership.");
				Engine.DestroyEntity(building);
				return;
			}
			owner = cmpOwnership.GetOwner();
		}
		var cmpBuildingOwnership = Engine.QueryInterface(building, IID_Ownership);
		if (!cmpBuildingOwnership)
		{
			error("New Building " + building + " has no ownership.");
			Engine.DestroyEntity(building);
			return;
		}
		cmpBuildingOwnership.SetOwner(owner);

		/*
		Copy over the obstruction control group IDs from the foundation
		entities. This is needed to ensure that when a foundation is completed
		and replaced by a new entity, it remains in the same control group(s)
		as any other foundation entities that may surround it. This is the
		mechanism that is used to e.g. enable wall pieces to be built closely
		together, ignoring their mutual obstruction shapes (since they would
		otherwise be prevented from being built so closely together). If the
		control groups are not copied over, the new entity will default to a
		new control group containing only itself, and will hence block
		construction of any surrounding foundations that it was previously in
		the same control group with.

		Note that this will result in the completed building entities having
		control group IDs that equal entity IDs of old (and soon to be deleted)
		foundation entities. This should not have any consequences, however,
		since the control group IDs are only meant to be unique identifiers,
		which is still true when reusing the old ones.
		*/

		var cmpBuildingObstruction = Engine.QueryInterface(building, IID_Obstruction);
		if (cmpObstruction && cmpBuildingObstruction)
		{
			cmpBuildingObstruction.SetControlGroup(cmpObstruction.GetControlGroup());
			cmpBuildingObstruction.SetControlGroup2(cmpObstruction.GetControlGroup2());
		}

		var cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
		if (cmpPlayerStatisticsTracker)
			cmpPlayerStatisticsTracker.IncreaseConstructedBuildingsCounter(building);

		var cmpBuildingHealth = Engine.QueryInterface(building, IID_Health);
		if (cmpBuildingHealth)
			cmpBuildingHealth.SetHitpoints(cmpHealth.GetHitpoints());

		PlaySound("constructed", building);

		Engine.PostMessage(this.entity, MT_ConstructionFinished,
			{ "entity": this.entity, "newentity": building });
		Engine.PostMessage(this.entity, MT_EntityRenamed, { "entity": this.entity, "newentity": building });

		Engine.DestroyEntity(this.entity);
	}
};

Foundation.prototype.GetBuildRate = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	// Return infinity for instant structure conversion
	return cmpHealth.GetMaxHitpoints() / cmpCost.GetBuildTime();
};

Engine.RegisterComponentType(IID_Foundation, "Foundation", Foundation);

