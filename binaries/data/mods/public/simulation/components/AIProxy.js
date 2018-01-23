function AIProxy() {}

AIProxy.prototype.Schema =
	"<empty/>";

/**
 * AIProxy passes its entity's state data to AI scripts.
 *
 * Efficiency is critical: there can be many thousands of entities,
 * and the data returned by this component is serialized and copied to
 * the AI thread every turn, so it can be quite expensive.
 *
 * We omit all data that can be derived statically from the template XML
 * files - the AI scripts can parse the templates themselves.
 * This violates the component interface abstraction and is potentially
 * fragile if the template formats change (since both the component code
 * and the AI will have to be updated in sync), but it's not *that* bad
 * really and it helps performance significantly.
 *
 * We also add an optimisation to avoid copying non-changing values.
 * The first call to GetRepresentation calls GetFullRepresentation,
 * which constructs the complete entity state representation.
 * After that, we simply listen to events from the rest of the gameplay code,
 * and store the changed data in this.changes.
 * Properties in this.changes will override those previously returned
 * from GetRepresentation; if a property isn't overridden then the AI scripts
 * will keep its old value.
 *
 * The event handlers should set this.changes.whatever to exactly the
 * same as GetFullRepresentation would set.
 */

AIProxy.prototype.Init = function()
{
	this.changes = null;
	this.needsFullGet = true;
	this.cmpAIInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIInterface);
};

AIProxy.prototype.Serialize = null; // we have no dynamic state to save

AIProxy.prototype.Deserialize = function ()
{
	this.Init();
};

AIProxy.prototype.GetRepresentation = function()
{
	// Return the full representation the first time we're called
	let ret;
	if (this.needsFullGet)
		ret = this.GetFullRepresentation();
	else
		ret = this.changes;

	// Initialise changes to null instead of {}, to avoid memory allocations in the
	// common case where there will be no changes; event handlers should each reset
	// it to {} if needed
	this.changes = null;

	return ret;
};

AIProxy.prototype.NotifyChange = function()
{
	if (this.needsFullGet)
	{
		// not yet notified, be sure that the owner is set before doing so
		// as the Create event is sent only on first ownership changed
		let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (!cmpOwnership || cmpOwnership.GetOwner() < 0)
			return false;
	}

	if (!this.changes)
	{
		this.changes = {};
		this.cmpAIInterface.ChangedEntity(this.entity);
	}
	return true;
};

// AI representation-updating event handlers:

AIProxy.prototype.OnPositionChanged = function(msg)
{
	if (!this.NotifyChange())
		return;

	if (msg.inWorld)
	{
		this.changes.position = [msg.x, msg.z];
		this.changes.angle = msg.a;
	}
	else
	{
		this.changes.position = undefined;
		this.changes.angle = undefined;
	}
};

AIProxy.prototype.OnHealthChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.hitpoints = msg.to;
};

AIProxy.prototype.OnCapturePointsChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.capturePoints = msg.capturePoints;
};

AIProxy.prototype.OnUnitIdleChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.idle = msg.idle;
};

AIProxy.prototype.OnUnitStanceChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.stance = msg.to;
};

AIProxy.prototype.OnUnitAIStateChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.unitAIState = msg.to;
};

AIProxy.prototype.OnUnitAIOrderDataChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.unitAIOrderData = msg.to;
};

AIProxy.prototype.OnProductionQueueChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	this.changes.trainingQueue = cmpProductionQueue.GetQueue();
};

AIProxy.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	if (!this.NotifyChange())
		return;

	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	this.changes.garrisoned = cmpGarrisonHolder.GetEntities();

	// Send a message telling a unit garrisoned or ungarrisoned.
	// I won't check if the unit is still alive so it'll be up to the AI.
	for (let ent of msg.added)
		this.cmpAIInterface.PushEvent("Garrison", {"entity" : ent, "holder": this.entity});
	for (let ent of msg.removed)
		this.cmpAIInterface.PushEvent("UnGarrison", {"entity" : ent, "holder": this.entity});
};

AIProxy.prototype.OnResourceSupplyChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.resourceSupplyAmount = msg.to;
};

AIProxy.prototype.OnResourceSupplyNumGatherersChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.resourceSupplyNumGatherers = msg.to;
};

AIProxy.prototype.OnResourceCarryingChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.resourceCarrying = msg.to;
};

AIProxy.prototype.OnFoundationProgressChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.foundationProgress = msg.to;
};

AIProxy.prototype.OnFoundationBuildersChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.foundationBuilders = msg.to;
};

AIProxy.prototype.OnDropsiteSharingChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.sharedDropsite = msg.shared;
};

AIProxy.prototype.OnTerritoryDecayChanged = function(msg)
{
	if (!this.NotifyChange())
		return;
	this.changes.decaying = msg.to;
	this.cmpAIInterface.PushEvent("TerritoryDecayChanged", msg);
};

// TODO: event handlers for all the other things

AIProxy.prototype.GetFullRepresentation = function()
{
	this.needsFullGet = false;
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	let ret = {
		// These properties are constant and won't need to be updated
		"id": this.entity,
		"template": cmpTemplateManager.GetCurrentTemplateName(this.entity)
	};

	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
	{
		// Updated by OnPositionChanged

		if (cmpPosition.IsInWorld())
		{
			let pos = cmpPosition.GetPosition2D();
			ret.position = [pos.x, pos.y];
			ret.angle = cmpPosition.GetRotation().y;
		}
		else
		{
			ret.position = undefined;
			ret.angle = undefined;
		}
	}

	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth)
	{
		// Updated by OnHealthChanged
		ret.hitpoints = cmpHealth.GetHitpoints();
	}

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
	{
		// Updated by OnOwnershipChanged
		ret.owner = cmpOwnership.GetOwner();
	}

	let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	if (cmpUnitAI)
	{
		// Updated by OnUnitIdleChanged
		ret.idle = cmpUnitAI.IsIdle();
		// Updated by OnUnitStanceChanged
		ret.stance = cmpUnitAI.GetStanceName();
		// Updated by OnUnitAIStateChanged
		ret.unitAIState = cmpUnitAI.GetCurrentState();
		// Updated by OnUnitAIOrderDataChanged
		ret.unitAIOrderData = cmpUnitAI.GetOrderData();
	}

	let cmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (cmpProductionQueue)
	{
		// Updated by OnProductionQueueChanged
		ret.trainingQueue = cmpProductionQueue.GetQueue();
	}

	let cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
	{
		// Updated by OnFoundationProgressChanged
		ret.foundationProgress = cmpFoundation.GetBuildPercentage();
	}

	let cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
	{
		// Updated by OnResourceSupplyChanged
		ret.resourceSupplyAmount = cmpResourceSupply.GetCurrentAmount();
		ret.resourceSupplyNumGatherers = cmpResourceSupply.GetNumGatherers();
	}

	let cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		// Updated by OnResourceCarryingChanged
		ret.resourceCarrying = cmpResourceGatherer.GetCarryingStatus();
	}

	let cmpResourceDropsite = Engine.QueryInterface(this.entity, IID_ResourceDropsite);
	if (cmpResourceDropsite)
	{
		// Updated by OnDropsiteSharingChanged
		ret.sharedDropsite = cmpResourceDropsite.IsShared();
	}

	let cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		// Updated by OnGarrisonedUnitsChanged
		ret.garrisoned = cmpGarrisonHolder.GetEntities();
	}

	let cmpTerritoryDecay = Engine.QueryInterface(this.entity, IID_TerritoryDecay);
	if (cmpTerritoryDecay)
		ret.decaying = cmpTerritoryDecay.IsDecaying();

	let cmpCapturable = Engine.QueryInterface(this.entity, IID_Capturable);
	if (cmpCapturable)
		ret.capturePoints = cmpCapturable.GetCapturePoints();

	return ret;
};

// AI event handlers:
// (These are passed directly as events to the AI scripts, rather than updating
// our proxy representation.)
// (This shouldn't include extremely high-frequency events, like PositionChanged,
// because that would be very expensive and AI will rarely care about all those
// events.)

// special case: this changes the state and sends an event.
AIProxy.prototype.OnOwnershipChanged = function(msg)
{
	this.NotifyChange();

	if (msg.from == INVALID_PLAYER)
	{
		this.cmpAIInterface.PushEvent("Create", {"entity" : msg.entity});
		return;
	}
	else if (msg.to == INVALID_PLAYER)
	{
		this.cmpAIInterface.PushEvent("Destroy", {"entity" : msg.entity});
		this.needsFullGet = true;
		return;
	}

	this.changes.owner = msg.to;
	this.cmpAIInterface.PushEvent("OwnershipChanged", msg);
};

AIProxy.prototype.OnAttacked = function(msg)
{
	this.cmpAIInterface.PushEvent("Attacked", msg);
};

AIProxy.prototype.OnConstructionFinished = function(msg)
{
	this.cmpAIInterface.PushEvent("ConstructionFinished", msg);
};

AIProxy.prototype.OnTrainingStarted = function(msg)
{
	this.cmpAIInterface.PushEvent("TrainingStarted", msg);
};

AIProxy.prototype.OnTrainingFinished = function(msg)
{
	this.cmpAIInterface.PushEvent("TrainingFinished", msg);
};

AIProxy.prototype.OnAIMetadata = function(msg)
{
	this.cmpAIInterface.PushEvent("AIMetadata", msg);
};

Engine.RegisterComponentType(IID_AIProxy, "AIProxy", AIProxy);
