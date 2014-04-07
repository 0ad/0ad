function Promotion() {}

Promotion.prototype.Schema =
	"<element name='Entity'>" +
		"<text/>" +
	"</element>" +
	"<element name='RequiredXp'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

Promotion.prototype.Init = function()
{
	this.currentXp = 0;
};

Promotion.prototype.GetRequiredXp = function()
{
	return ApplyValueModificationsToEntity("Promotion/RequiredXp", +this.template.RequiredXp, this.entity);
};

Promotion.prototype.GetCurrentXp = function()
{
	return this.currentXp;
};

Promotion.prototype.GetPromotedTemplateName = function()
{
	return this.template.Entity;
};

Promotion.prototype.Promote = function(promotedTemplateName)
{
	// Create promoted unit entity
	var promotedUnitEntity = Engine.AddEntity(promotedTemplateName);

	// Copy parameters from current entity to promoted one
	var cmpCurrentUnitPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpPromotedUnitPosition = Engine.QueryInterface(promotedUnitEntity, IID_Position);
	if (cmpCurrentUnitPosition.IsInWorld())
	{
		var pos = cmpCurrentUnitPosition.GetPosition2D();
		cmpPromotedUnitPosition.JumpTo(pos.x, pos.y);
	}
	var rot = cmpCurrentUnitPosition.GetRotation();
	cmpPromotedUnitPosition.SetYRotation(rot.y);
	cmpPromotedUnitPosition.SetXZRotation(rot.x, rot.z);
	var heightOffset = cmpCurrentUnitPosition.GetHeightOffset();
	cmpPromotedUnitPosition.SetHeightOffset(heightOffset);

	var cmpCurrentUnitOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpPromotedUnitOwnership = Engine.QueryInterface(promotedUnitEntity, IID_Ownership);
	cmpPromotedUnitOwnership.SetOwner(cmpCurrentUnitOwnership.GetOwner());

	// change promoted unit health to the same percent of hitpoints as unit had before promotion
	var cmpCurrentUnitHealth = Engine.QueryInterface(this.entity, IID_Health);
	var cmpPromotedUnitHealth = Engine.QueryInterface(promotedUnitEntity, IID_Health);
	var healthFraction = Math.max(0, Math.min(1, cmpCurrentUnitHealth.GetHitpoints() / cmpCurrentUnitHealth.GetMaxHitpoints()));
	var promotedUnitHitpoints = Math.round(cmpPromotedUnitHealth.GetMaxHitpoints() * healthFraction);
	cmpPromotedUnitHealth.SetHitpoints(promotedUnitHitpoints);

	var cmpPromotedUnitPromotion = Engine.QueryInterface(promotedUnitEntity, IID_Promotion);
	if (cmpPromotedUnitPromotion)
		cmpPromotedUnitPromotion.IncreaseXp(this.currentXp);

	var cmpCurrentUnitResourceGatherer = Engine.QueryInterface(this.entity, IID_ResourceGatherer);
	var cmpPromotedUnitResourceGatherer = Engine.QueryInterface(promotedUnitEntity, IID_ResourceGatherer);
	if (cmpCurrentUnitResourceGatherer && cmpPromotedUnitResourceGatherer)
	{
		var carriedResorces = cmpCurrentUnitResourceGatherer.GetCarryingStatus();
		cmpPromotedUnitResourceGatherer.GiveResources(carriedResorces);
	}

	var cmpCurrentUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
	var cmpPromotedUnitAI = Engine.QueryInterface(promotedUnitEntity, IID_UnitAI);
	cmpPromotedUnitAI.SetHeldPosition(cmpCurrentUnitAI.GetHeldPosition());
	if (cmpCurrentUnitAI.GetStanceName())
		cmpPromotedUnitAI.SwitchToStance(cmpCurrentUnitAI.GetStanceName());

	var orders = cmpCurrentUnitAI.GetOrders();
	if (cmpCurrentUnitAI.IsGarrisoned())
	{
		if (orders.length > 0 && orders[0].type == "Garrison")
		{
			// Replace the garrison order by an autogarrison order,
			// as we are already garrisoned and do not need to do
			// any further checks (or else we should do them here).
			orders.shift();
			cmpPromotedUnitAI.Autogarrison();
		}
		else
			warn("Promoted garrisoned entity with empty order queue.");
	}
	else
		cmpPromotedUnitAI.Cheer();

	cmpPromotedUnitAI.AddOrders(orders);

	var workOrders = cmpCurrentUnitAI.GetWorkOrders();
	cmpPromotedUnitAI.SetWorkOrders(workOrders);
	cmpPromotedUnitAI.SetGuardOf(cmpCurrentUnitAI.IsGuardOf());

	var cmpCurrentUnitGuard = Engine.QueryInterface(this.entity, IID_Guard);
	var cmpPromotedUnitGuard = Engine.QueryInterface(promotedUnitEntity, IID_Guard);
	if (cmpCurrentUnitGuard && cmpPromotedUnitGuard)
		cmpPromotedUnitGuard.SetEntities(cmpCurrentUnitGuard.GetEntities());

	Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: promotedUnitEntity });

	// Destroy current entity
	Engine.DestroyEntity(this.entity);
	// save the entity id
	this.promotedUnitEntity = promotedUnitEntity;
};

Promotion.prototype.IncreaseXp = function(amount)
{
	// if the unit was already promoted, but is waiting for the engine to be destroyed
	// transfer the gained xp to the promoted unit if applicable
	if (this.promotedUnitEntity)
	{
		var cmpPromotion = Engine.QueryInterface(this.promotedUnitEntity, IID_Promotion);
		if (cmpPromotion)
			cmpPromotion.IncreaseXp(amount);
		return;
	}

	this.currentXp += +(amount);
	var requiredXp = this.GetRequiredXp();

	if (this.currentXp >= requiredXp)
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var playerID = QueryOwnerInterface(this.entity, IID_Player).GetPlayerID();
		this.currentXp -= requiredXp;
		var promotedTemplateName = this.GetPromotedTemplateName();
		// check if we can upgrade a second time (or even more)
		while (true)
		{
			var template = cmpTemplateManager.GetTemplate(promotedTemplateName);
			if (!template.Promotion)
				break;
			requiredXp = ApplyValueModificationsToTemplate("Promotion/RequiredXp", +template.Promotion.RequiredXp, playerID, template);
			// compare the current xp to the required xp of the promoted entity
			if (this.currentXp < requiredXp)
				break;
			this.currentXp -= requiredXp;
			promotedTemplateName = template.Promotion.Entity;
		}
		this.Promote(promotedTemplateName);
	}
};

Promotion.prototype.OnValueModification = function(msg)
{
	if (msg.component == "Promotion")
		this.IncreaseXp(0);
};

Engine.RegisterComponentType(IID_Promotion, "Promotion", Promotion);