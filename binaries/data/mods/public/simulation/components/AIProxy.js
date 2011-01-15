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
};

AIProxy.prototype.GetRepresentation = function()
{
	// Return the full representation the first time we're called
	var ret;
	if (this.needsFullGet)
	{
		ret = this.GetFullRepresentation();
		this.needsFullGet = false;
	}
	else
	{
		ret = this.changes;
	}

	// Initialise changes to null instead of {}, to avoid memory allocations in the
	// common case where there will be no changes; event handlers should each reset
	// it to {} if needed
	this.changes = null;

	return ret;
};

AIProxy.prototype.OnPositionChanged = function(msg)
{
	if (!this.changes)
		this.changes = {};

	if (msg.inWorld)
		this.changes.position = [msg.x, msg.z];
	else
		this.changes.position = undefined;
};

AIProxy.prototype.OnHealthChanged = function(msg)
{
	if (!this.changes)
		this.changes = {};

	this.changes.hitpoints = msg.to;
};

AIProxy.prototype.OnOwnershipChanged = function(msg)
{
	if (!this.changes)
		this.changes = {};

	this.changes.owner = msg.to;
};

// TODO: event handlers for all the other things

AIProxy.prototype.GetFullRepresentation = function()
{
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	var ret = {
		// These properties are constant and won't need to be updated
		"id": this.entity,
		"template": cmpTemplateManager.GetCurrentTemplateName(this.entity)
	}

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (cmpPosition)
	{
		// Updated by OnPositionChanged

		if (cmpPosition.IsInWorld())
		{
			var pos = cmpPosition.GetPosition2D();
			ret.position = [pos.x, pos.y];
		}
		else
		{
			ret.position = undefined;
		}
	}

	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth)
	{
		// Updated by OnHealthChanged
		ret.hitpoints = cmpHealth.GetHitpoints();
	}

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
	{
		// Updated by OnOwnershipChanged
		ret.owner = cmpOwnership.GetOwner();
	}

	var cmpTrainingQueue = Engine.QueryInterface(this.entity, IID_TrainingQueue);
	if (cmpTrainingQueue)
	{
		ret.trainingQueue = cmpTrainingQueue.GetQueue();
	}

	var cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
	{
		ret.foundationProgress = cmpFoundation.GetBuildPercentage();
	}

	var cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	if (cmpResourceSupply)
	{
		ret.resourceSupplyAmount = cmpResourceSupply.GetCurrentAmount();
	}

	var cmpResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	if (cmpResourceGatherer)
	{
		ret.resourceCarrying = cmpResourceGatherer.GetCarryingStatus();
	}

	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		ret.garrisoned = cmpGarrisonHolder.GetEntities();
	}

	return ret;
};

Engine.RegisterComponentType(IID_AIProxy, "AIProxy", AIProxy);
