function Foundation() {}

Foundation.prototype.Schema =
	"<element name='BuildTimeModifier' a:help='Effect for having multiple builders.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

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

	this.buildTimeModifier = +this.template.BuildTimeModifier;

	this.previewEntity = INVALID_ENTITY;
};

Foundation.prototype.Serialize = function()
{
	let ret = Object.assign({}, this);
	ret.previewEntity = INVALID_ENTITY;
	return ret;
};

Foundation.prototype.Deserialize = function(data)
{
	this.Init();
	Object.assign(this, data);
};

Foundation.prototype.OnDeserialized = function()
{
	this.CreateConstructionPreview();
};

Foundation.prototype.InitialiseConstruction = function(template)
{
	this.finalTemplateName = template;

	// Remember the cost here, so if it changes after construction begins (from auras or technologies)
	// we will use the correct values to refund partial construction costs.
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	if (!cmpCost)
		error("A foundation, from " + template + ", must have a cost component to know the build time");

	this.costs = cmpCost.GetResourceCosts();

	this.maxProgress = 0;

	this.initialised = true;
};

/**
 * Moving the revelation logic from Build to here makes the building sink if
 * it is attacked.
 */
Foundation.prototype.OnHealthChanged = function(msg)
{
	let cmpPosition = Engine.QueryInterface(this.previewEntity, IID_Position);
	if (cmpPosition)
		cmpPosition.SetConstructionProgress(this.GetBuildProgress());

	Engine.PostMessage(this.entity, MT_FoundationProgressChanged, { "to": this.GetBuildPercentage() });
};

/**
 * Returns the current build progress in a [0,1] range.
 */
Foundation.prototype.GetBuildProgress = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
		return 0;

	return cmpHealth.GetHitpoints() / cmpHealth.GetMaxHitpoints();
};

Foundation.prototype.GetBuildPercentage = function()
{
	return Math.floor(this.GetBuildProgress() * 100);
};

/**
 * @return {number[]} - An array containing the entity IDs of assigned builders.
 */
Foundation.prototype.GetBuilders = function()
{
	return Array.from(this.builders.keys());
};

Foundation.prototype.GetNumBuilders = function()
{
	return this.builders.size;
};

Foundation.prototype.IsFinished = function()
{
	return (this.GetBuildProgress() == 1.0);
};

Foundation.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER && this.previewEntity != INVALID_ENTITY)
	{
		let cmpPreviewOwnership = Engine.QueryInterface(this.previewEntity, IID_Ownership);
		if (cmpPreviewOwnership)
			cmpPreviewOwnership.SetOwner(msg.to);
		return;
	}

	if (msg.to != INVALID_PLAYER || !this.initialised)
		return;

	if (this.previewEntity != INVALID_ENTITY)
	{
		Engine.DestroyEntity(this.previewEntity);
		this.previewEntity = INVALID_ENTITY;
	}

	if (this.IsFinished())
		return;

	let cmpPlayer = QueryPlayerIDInterface(msg.from);
	let cmpStatisticsTracker = QueryPlayerIDInterface(msg.from, IID_StatisticsTracker);

	// Refund a portion of the construction cost, proportional
	// to the amount of build progress remaining.
	for (let r in this.costs)
	{
		let scaled = Math.ceil(this.costs[r] * (1.0 - this.maxProgress));
		if (scaled)
		{
			if (cmpPlayer)
				cmpPlayer.AddResource(r, scaled);
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseResourceUsedCounter(r, -scaled);
		}
	}
};

/**
 * @param {number[]} builders - An array containing the entity IDs of builders to assign.
 */
Foundation.prototype.AddBuilders = function(builders)
{
	let changed = false;
	for (let builder of builders)
		changed = this.AddBuilderHelper(builder) || changed;

	if (changed)
		this.HandleBuildersChanged();
};

/**
 * @param {number} builderEnt - The entity to add.
 * @return {boolean} - Whether the addition was successful.
 */
Foundation.prototype.AddBuilderHelper = function(builderEnt)
{
	if (this.builders.has(builderEnt))
		return false;

	let cmpBuilder = Engine.QueryInterface(builderEnt, IID_Builder) ||
		Engine.QueryInterface(this.entity, IID_AutoBuildable);
	if (!cmpBuilder)
		return false;

	let buildRate = cmpBuilder.GetRate();
	this.builders.set(builderEnt, buildRate);
	this.totalBuilderRate += buildRate;

	return true;
};

/**
 * @param {number} builderEnt - The entity to add.
 */
Foundation.prototype.AddBuilder = function(builderEnt)
{
	if (this.AddBuilderHelper(builderEnt))
		this.HandleBuildersChanged();
};

/**
 * @param {number} builderEnt - The entity to remove.
 */
Foundation.prototype.RemoveBuilder = function(builderEnt)
{
	if (!this.builders.has(builderEnt))
		return;

	this.totalBuilderRate -= this.builders.get(builderEnt);
	this.builders.delete(builderEnt);
	this.HandleBuildersChanged();
};

/**
 * This has to be called whenever the number of builders change.
 */
Foundation.prototype.HandleBuildersChanged = function()
{
	this.SetBuildMultiplier();

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SetVariable("numbuilders", this.GetNumBuilders());

	Engine.PostMessage(this.entity, MT_FoundationBuildersChanged, { "to": this.GetBuilders() });
};

/**
 * The build multiplier is a penalty that is applied to each builder.
 * For example, ten women build at a combined rate of 10^0.7 = 5.01 instead of 10.
 */
Foundation.prototype.CalculateBuildMultiplier = function(num)
{
	// Avoid division by zero, in particular 0/0 = NaN which isn't reliably serialized
	return num < 2 ? 1 : Math.pow(num, this.buildTimeModifier) / num;
};

Foundation.prototype.SetBuildMultiplier = function()
{
	this.buildMultiplier = this.CalculateBuildMultiplier(this.GetNumBuilders());
};

Foundation.prototype.GetBuildTime = function()
{
	let timeLeft = (1 - this.GetBuildProgress()) * Engine.QueryInterface(this.entity, IID_Cost).GetBuildTime();
	let rate = this.totalBuilderRate * this.buildMultiplier;
	let rateNew = (this.totalBuilderRate + 1) * this.CalculateBuildMultiplier(this.GetNumBuilders() + 1);
	return {
		// Avoid division by zero, in particular 0/0 = NaN which isn't reliably serialized
		"timeRemaining": rate ? timeLeft / rate : 0,
		"timeRemainingNew": timeLeft / rateNew
	};
};

/**
 * @return {boolean} - Whether the foundation has been committed sucessfully.
 */
Foundation.prototype.Commit = function()
{
	if (this.committed)
		return false;

	let cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction && cmpObstruction.GetBlockMovementFlag(true))
	{
		for (let ent of cmpObstruction.GetEntitiesDeletedUponConstruction())
			Engine.DestroyEntity(ent);

		let collisions = cmpObstruction.GetEntitiesBlockingConstruction();
		if (collisions.length)
		{
			for (let ent of collisions)
			{
				let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
				if (cmpUnitAI)
					cmpUnitAI.LeaveFoundation(this.entity);

				// TODO: What if an obstruction has no UnitAI?
			}

			// TODO: maybe we should tell the builder to use a special
			// animation to indicate they're waiting for people to get
			// out the way

			return false;
		}
	}

	// The obstruction always blocks new foundations/construction,
	// but we've temporarily allowed units to walk all over it
	// (via CCmpTemplateManager). Now we need to remove that temporary
	// blocker-disabling, so that we'll perform standard unit blocking instead.
	if (cmpObstruction)
		cmpObstruction.SetDisableBlockMovementPathfinding(false, false, -1);

	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.CallEvent("OnConstructionStarted", {
		"foundation": this.entity,
		"template": this.finalTemplateName
	});

	let cmpFoundationVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpFoundationVisual)
		cmpFoundationVisual.SelectAnimation("scaffold", false, 1.0);

	this.committed = true;
	this.CreateConstructionPreview();
	return true;
};

/**
 * Perform some number of seconds of construction work.
 * Returns true if the construction is completed.
 */
Foundation.prototype.Build = function(builderEnt, work)
{
	// Do nothing if we've already finished building
	// (The entity will be destroyed soon after completion so
	// this won't happen much.)
	if (this.IsFinished())
		return;

	if (!this.committed && !this.Commit())
		return;

	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
	{
		error("Foundation " + this.entity + " does not have a health component.");
		return;
	}
	let deltaHP = work * this.GetBuildRate() * this.buildMultiplier;
	if (deltaHP > 0)
		cmpHealth.Increase(deltaHP);

	// Update the total builder rate.
	this.totalBuilderRate += work - this.builders.get(builderEnt);
	this.builders.set(builderEnt, work);

	// Remember our max progress for partial refund in case of destruction.
	this.maxProgress = Math.max(this.maxProgress, this.GetBuildProgress());

	if (this.maxProgress >= 1.0)
	{
		let cmpPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);

		let building = ChangeEntityTemplate(this.entity, this.finalTemplateName);

		const cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
		const cmpBuildingIdentity = Engine.QueryInterface(building, IID_Identity);
		if (cmpIdentity && cmpBuildingIdentity)
		{
			const oldPhenotype = cmpIdentity.GetPhenotype();
			if (cmpBuildingIdentity.GetPhenotype() !== oldPhenotype)
			{
				cmpBuildingIdentity.SetPhenotype(oldPhenotype);
				Engine.QueryInterface(building, IID_Visual)?.RecomputeActorName();
			}
		}

		if (cmpPlayerStatisticsTracker)
			cmpPlayerStatisticsTracker.IncreaseConstructedBuildingsCounter(building);

		PlaySound("constructed", building);

		Engine.PostMessage(this.entity, MT_ConstructionFinished,
			{ "entity": this.entity, "newentity": building });

		for (let builder of this.GetBuilders())
		{
			let cmpUnitAIBuilder = Engine.QueryInterface(builder, IID_UnitAI);
			if (cmpUnitAIBuilder)
				cmpUnitAIBuilder.ConstructionFinished({ "entity": this.entity, "newentity": building });
		}
	}
};

Foundation.prototype.GetBuildRate = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	// Return infinity for instant structure conversion
	return cmpHealth.GetMaxHitpoints() / cmpCost.GetBuildTime();
};

/**
 * Create preview entity and copy various parameters from the foundation.
 */
Foundation.prototype.CreateConstructionPreview = function()
{
	if (this.previewEntity)
	{
		Engine.DestroyEntity(this.previewEntity);
		this.previewEntity = INVALID_ENTITY;
	}

	if (!this.committed)
		return;

	let cmpFoundationVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (!cmpFoundationVisual || !cmpFoundationVisual.HasConstructionPreview())
		return;

	this.previewEntity = Engine.AddLocalEntity("construction|"+this.finalTemplateName);
	let cmpFoundationOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpPreviewOwnership = Engine.QueryInterface(this.previewEntity, IID_Ownership);
	if (cmpFoundationOwnership && cmpPreviewOwnership)
		cmpPreviewOwnership.SetOwner(cmpFoundationOwnership.GetOwner());

	// TODO: the 'preview' would be invisible if it doesn't have the below component,
	// Maybe it makes more sense to simply delete it then?

	// Initially hide the preview underground
	let cmpPreviewPosition = Engine.QueryInterface(this.previewEntity, IID_Position);
	let cmpFoundationPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPreviewPosition && cmpFoundationPosition)
	{
		let rot = cmpFoundationPosition.GetRotation();
		cmpPreviewPosition.SetYRotation(rot.y);
		cmpPreviewPosition.SetXZRotation(rot.x, rot.z);

		let pos = cmpFoundationPosition.GetPosition2D();
		cmpPreviewPosition.JumpTo(pos.x, pos.y);

		cmpPreviewPosition.SetConstructionProgress(this.GetBuildProgress());
	}

	let cmpPreviewVisual = Engine.QueryInterface(this.previewEntity, IID_Visual);
	if (cmpPreviewVisual && cmpFoundationVisual)
	{
		cmpPreviewVisual.SetActorSeed(cmpFoundationVisual.GetActorSeed());
		cmpPreviewVisual.SelectAnimation("scaffold", false, 1.0);
	}
};

Foundation.prototype.OnEntityRenamed = function(msg)
{
	let cmpFoundationNew = Engine.QueryInterface(msg.newentity, IID_Foundation);
	if (cmpFoundationNew)
		cmpFoundationNew.AddBuilders(this.GetBuilders());
};

function FoundationMirage() {}
FoundationMirage.prototype.Init = function(cmpFoundation)
{
	this.numBuilders = cmpFoundation.GetNumBuilders();
	this.buildTime = cmpFoundation.GetBuildTime();
};

FoundationMirage.prototype.GetNumBuilders = function() { return this.numBuilders; };
FoundationMirage.prototype.GetBuildTime = function() { return this.buildTime; };

Engine.RegisterGlobal("FoundationMirage", FoundationMirage);

Foundation.prototype.Mirage = function()
{
	let mirage = new FoundationMirage();
	mirage.Init(this);
	return mirage;
};

Engine.RegisterComponentType(IID_Foundation, "Foundation", Foundation);
