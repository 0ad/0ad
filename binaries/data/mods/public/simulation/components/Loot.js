function Loot() {}

Loot.prototype.Schema =
	"<a:help>Specifies the loot credited when this entity is killed.</a:help>" +
	"<a:example>" +
		"<xp>35</xp>" +
		"<metal>10</metal>" +
	"</a:example>" +
	Resources.BuildSchema("nonNegativeInteger", ["xp"]);

Loot.prototype.Serialize = null; // we have no dynamic state to save

Loot.prototype.GetXp = function()
{
	return Math.floor(ApplyValueModificationsToEntity("Loot/xp", +(this.template.xp || 0), this.entity));
};

Loot.prototype.GetResources = function()
{
	let ret = {};
	for (let res of Resources.GetCodes())
		ret[res] = Math.floor(ApplyValueModificationsToEntity("Loot/" + res, +(this.template[res] || 0), this.entity));
	return ret;
};

Engine.RegisterComponentType(IID_Loot, "Loot", Loot);
