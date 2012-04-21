function Armour() {}

Armour.prototype.Schema =
	"<a:help>Controls the damage resistance of the unit.</a:help>" +
	"<a:example>" +
		"<Hack>10.0</Hack>" +
		"<Pierce>0.0</Pierce>" +
		"<Crush>5.0</Crush>" +
	"</a:example>" +
	"<element name='Hack' a:help='Hack damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Pierce' a:help='Pierce damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Crush' a:help='Crush damage protection'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Armour.prototype.Init = function()
{
	this.invulnerable = false;
};

Armour.prototype.Serialize = null; // we have no dynamic state to save

Armour.prototype.SetInvulnerability = function(invulnerability)
{
	this.invulnerable = invulnerability;
};

Armour.prototype.TakeDamage = function(hack, pierce, crush)
{
	if (this.invulnerable) 
		return { "killed": false };
	// Adjust damage values based on armour
	var adjHack = Math.max(0, hack - this.template.Hack);
	var adjPierce = Math.max(0, pierce - this.template.Pierce);
	var adjCrush = Math.max(0, crush - this.template.Crush);

	// Total is sum of individual damages, with minimum damage 1
	//	Round to nearest integer, since HP is integral
	var total = Math.max(1, Math.round(adjHack + adjPierce + adjCrush));

	// Reduce health
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	return cmpHealth.Reduce(total);
};

Armour.prototype.GetArmourStrengths = function()
{
	if (!this.armourCache)
	{
		// Work out the armour values with technology effects
		var self = this;
		
		var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
		var applyTechs = function(type)
		{
			var allComponent = cmpTechMan.ApplyModifications("Armour/All", +self.template[type], self.entity) - self.template[type];
			return allComponent + cmpTechMan.ApplyModifications("Armour/" + type, +self.template[type], self.entity);
		};
		
		this.armourCache = {
			hack: applyTechs("Hack"),
			pierce: applyTechs("Pierce"),
			crush: applyTechs("Crush")
		};
	}
	
	return this.armourCache;
};

// Remove any cached template data which is based on technology data
Armour.prototype.OnTechnologyModificationChange = function(msg)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return;
	
	var player = cmpOwnership.GetOwner();
	
	if (msg.component === "Armour" && msg.player === player)
		delete this.armourCache;
};

// Remove any cached template data which is based on technology data
Armour.prototype.OnOwnershipChanged = function(msg)
{
	delete this.armourCache;
};

Engine.RegisterComponentType(IID_DamageReceiver, "Armour", Armour);
