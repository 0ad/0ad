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

	// Collect resources carried by workers and traders
	var cmpResourceGatherer = Engine.QueryInterface(targetEntity, IID_ResourceGatherer);
	var cmpTrader = Engine.QueryInterface(targetEntity, IID_Trader);

	let resourcesCarried = calculateCarriedResources(
		cmpResourceGatherer && cmpResourceGatherer.GetCarryingStatus(),
		cmpTrader && cmpTrader.GetGoods()
	);

	// Loot resources as defined in the templates
	var resources = cmpLoot.GetResources();
	for (let type in resources)
		resources[type] = ApplyValueModificationsToEntity("Looter/Resource/"+type, resources[type], this.entity)
			+ (resourcesCarried[type] || 0);

	// TODO: stop assuming that cmpLoot.GetResources() delivers all resource types (by defining them in a central location)

	// Transfer resources
	var cmpPlayer = QueryOwnerInterface(this.entity);
	cmpPlayer.AddResources(resources);

	// Update statistics
	var cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseLootCollectedCounter(resources);
};

Engine.RegisterComponentType(IID_Looter, "Looter", Looter);
