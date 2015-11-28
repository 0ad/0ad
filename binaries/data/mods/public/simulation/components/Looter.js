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

	var xp = cmpLoot.GetXp();
	if (xp > 0)
	{
		var cmpPromotion = Engine.QueryInterface(this.entity, IID_Promotion);
		if (cmpPromotion)
			cmpPromotion.IncreaseXp(xp);
	}
	var cmpPlayer = QueryOwnerInterface(this.entity);
	var resources = cmpLoot.GetResources();
	for (var type in resources)
	{
		resources[type] = ApplyValueModificationsToEntity("Looter/Resource/"+type, resources[type], this.entity);
	}
	cmpPlayer.AddResources(resources);

	let cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseLootCollectedCounter(resources);

	// If target entity has trader component, add carried goods to loot too
	var cmpTrader = Engine.QueryInterface(targetEntity, IID_Trader);
	if (cmpTrader)
	{
		var carriedGoods = cmpTrader.GetGoods();
		if (carriedGoods.amount && carriedGoods.amount.traderGain)
		{
			// Convert from {type:<type>,amount:<amount>} to {<type>:<amount>}
			var resourcesToAdd = {};
			resourcesToAdd[carriedGoods.type] = carriedGoods.amount.traderGain;
			if (carriedGoods.amount.market1Gain)
				resourcesToAdd[carriedGoods.type] += carriedGoods.amount.market1Gain;
			if (carriedGoods.amount.market2Gain)
				resourcesToAdd[carriedGoods.type] += carriedGoods.amount.market2Gain;
			cmpPlayer.AddResources(resourcesToAdd);

			let cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
			if (cmpStatisticsTracker)
				cmpStatisticsTracker.IncreaseLootCollectedCounter(resourcesToAdd);
		}
	}
};

Engine.RegisterComponentType(IID_Looter, "Looter", Looter);
