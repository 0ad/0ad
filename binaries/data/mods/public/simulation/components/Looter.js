function Looter() {}

Looter.prototype.Schema =
	"<empty/>";

Looter.prototype.Serialize = null; // We have no dynamic state to save

/**
 * Try to collect loot from target entity
 */
Looter.prototype.Collect = function(targetEntity)
{
	var cmpLoot = Engine.QueryInterface(targetEntity, IID_Loot);
	if (!cmpLoot)
		return;

	// Loot experience points as defined in the template
	var xp = cmpLoot.GetXp();
	if (xp > 0)
	{
		let cmpPromotion = Engine.QueryInterface(this.entity, IID_Promotion);
		if (cmpPromotion)
			cmpPromotion.IncreaseXp(xp);
	}

	// Loot resources as defined in the templates
	var resources = cmpLoot.GetResources();
	for (let type in resources)
		resources[type] = ApplyValueModificationsToEntity("Looter/Resource/"+type, resources[type], this.entity);

	// TODO: stop assuming that cmpLoot.GetResources() delivers all resource types (by defining them in a central location)

	// Loot resources that killed enemies carried
	var cmpResourceGatherer = Engine.QueryInterface(targetEntity, IID_ResourceGatherer);
	if (cmpResourceGatherer)
		for (let resource of cmpResourceGatherer.GetCarryingStatus())
			resources[resource.type] += resource.amount;

	// Loot resources traders carry
	var cmpTrader = Engine.QueryInterface(targetEntity, IID_Trader);
	if (cmpTrader)
	{
		let carriedGoods = cmpTrader.GetGoods();
		if (carriedGoods.amount)
		{
			resources[carriedGoods.type] +=
				+ (carriedGoods.amount.traderGain || 0);
				+ (carriedGoods.amount.market1Gain || 0);
				+ (carriedGoods.amount.market2Gain || 0);
		}
	}

	// Transfer resources
	var cmpPlayer = QueryOwnerInterface(this.entity);
	cmpPlayer.AddResources(resources);

	// Update statistics
	var cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseLootCollectedCounter(resources);
};

Engine.RegisterComponentType(IID_Looter, "Looter", Looter);
