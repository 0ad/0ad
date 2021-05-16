function Promotion() {}

Promotion.prototype.Schema =
	"<element name='Entity'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='TrickleRate' a:help='Trickle of XP gained each second.'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='RequiredXp'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

Promotion.prototype.Init = function()
{
	this.currentXp = 0;
	this.ComputeTrickleRate();
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
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (cmpHealth && cmpHealth.GetHitpoints() == 0)
	{
		this.promotedUnitEntity = INVALID_ENTITY;
		return;
	}

	ChangeEntityTemplate(this.entity, promotedTemplateName);
};

/**
 * @param {number} entity - The entity ID of the entity that this unit has promoted to.
 */
Promotion.prototype.SetPromotedEntity = function(entity)
{
	this.promotedUnitEntity = entity;
};

Promotion.prototype.IncreaseXp = function(amount)
{
	// if the unit was already promoted, but is waiting for the engine to be destroyed
	// transfer the gained xp to the promoted unit if applicable
	if (this.promotedUnitEntity)
	{
		let cmpPromotion = Engine.QueryInterface(this.promotedUnitEntity, IID_Promotion);
		if (cmpPromotion)
			cmpPromotion.IncreaseXp(amount);
		return;
	}

	this.currentXp += +amount;
	let requiredXp = this.GetRequiredXp();

	if (this.currentXp >= requiredXp)
	{
		let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
		let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
		if (!cmpPlayer)
			return;

		let playerID = cmpPlayer.GetPlayerID();
		this.currentXp -= requiredXp;
		let promotedTemplateName = this.GetPromotedTemplateName();
		// check if we can upgrade a second time (or even more)
		while (true)
		{
			let template = cmpTemplateManager.GetTemplate(promotedTemplateName);
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

	Engine.PostMessage(this.entity, MT_ExperienceChanged, {});
};

Promotion.prototype.ComputeTrickleRate = function()
{
	this.trickleRate = ApplyValueModificationsToEntity("Promotion/TrickleRate", +(this.template.TrickleRate || 0), this.entity);
	this.CheckTrickleTimer();
};

Promotion.prototype.CheckTrickleTimer = function()
{
	if (!this.trickleRate)
	{
		if (this.trickleTimer)
		{
			let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.CancelTimer(this.trickleTimer);
			delete this.trickleTimer;
		}
		return;
	}

	if (this.trickleTimer)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.trickleTimer = cmpTimer.SetInterval(this.entity, IID_Promotion, "TrickleTick", 1000, 1000, null);
};

Promotion.prototype.TrickleTick = function()
{
	this.IncreaseXp(this.trickleRate);
};

Promotion.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Promotion")
		return;

	this.ComputeTrickleRate();
	this.IncreaseXp(0);
};

Engine.RegisterComponentType(IID_Promotion, "Promotion", Promotion);
