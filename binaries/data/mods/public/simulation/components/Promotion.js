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
}

Promotion.prototype.GetRequiredXp = function()
{
	return ApplyTechModificationsToEntity("Promotion/RequiredXp", +this.template.RequiredXp, this.entity);
};

Promotion.prototype.GetCurrentXp = function()
{
	return this.currentXp;
};

Promotion.prototype.GetPromotedTemplateName = function()
{
	return this.template.Entity;
}

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
	cmpPromotedUnitAI.Cheer();
	var orders = cmpCurrentUnitAI.GetOrders();
	cmpPromotedUnitAI.AddOrders(orders);

	Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: promotedUnitEntity });

	// Destroy current entity
	Engine.DestroyEntity(this.entity);
}

Promotion.prototype.IncreaseXp = function(amount)
{
	this.currentXp += +(amount);

	if (this.currentXp >= this.GetRequiredXp())
	{
		var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		var promotionTemplate = this.template;
		var promotedTemplateName;
		var requiredXp;

		// We may be able to promote by skipping over multiple templates
		//	so find the highest level we can reach
		do
		{
			requiredXp = this.GetRequiredXp();
			this.currentXp -= requiredXp;
			promotedTemplateName = promotionTemplate.Entity;
			var template = cmpTemplateManager.GetTemplate(promotedTemplateName);
			promotionTemplate = template.Promotion;
		}
		while (promotionTemplate && this.currentXp >= requiredXp);

		this.Promote(promotedTemplateName);
	}
}

Engine.RegisterComponentType(IID_Promotion, "Promotion", Promotion);
