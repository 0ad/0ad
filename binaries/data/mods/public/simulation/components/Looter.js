function Looter() {}

Looter.prototype.Schema =
	"<empty/>";

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
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	cmpPlayer.AddResources(cmpLoot.GetResources());
}

Engine.RegisterComponentType(IID_Looter, "Looter", Looter);

