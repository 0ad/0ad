function Heal() {}

Heal.prototype.Schema = 
	"<a:help>Controls the healing abilities of the unit.</a:help>" +
	"<a:example>" +
		"<Range>20</Range>" +
		"<HP>5</HP>" +
		"<Rate>2000</Rate>" +
		"<UnhealableClasses datatype=\"tokens\">Cavalry</UnhealableClasses>" +
		"<HealableClasses datatype=\"tokens\">Support Infantry</HealableClasses>" +
	"</a:example>" +
	"<element name='Range' a:help='Range (in metres) where healing is possible'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='HP' a:help='Hitpoints healed per Rate'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Rate' a:help='A heal is performed every Rate ms'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='UnhealableClasses' a:help='If the target has any of these classes it can not be healed (even if it has a class from HealableClasses)'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='HealableClasses' a:help='The target must have one of these classes to be healable'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>";

Heal.prototype.Init = function()
{
};

Heal.prototype.Serialize = null; // we have no dynamic state to save

Heal.prototype.GetTimers = function()
{
	var prepare = 1000;
	var repeat = +this.template.Rate;

	repeat = ApplyValueModificationsToEntity("Heal/Rate", repeat, this.entity);
	
	return { "prepare": prepare, "repeat": repeat };
};

Heal.prototype.GetRange = function()
{
	var min = 0;
	var max = +this.template.Range;
	
	max = ApplyValueModificationsToEntity("Heal/Range", max, this.entity);

	return { "max": max, "min": min };
};

Heal.prototype.GetUnhealableClasses = function()
{
	return this.template.UnhealableClasses._string;
};

Heal.prototype.GetHealableClasses = function()
{
	return this.template.HealableClasses._string;
};

/**
 * Heal the target entity. This should only be called after a successful range 
 * check, and should only be called after GetTimers().repeat msec has passed 
 * since the last call to PerformHeal.
 */
Heal.prototype.PerformHeal = function(target)
{
	var cmpHealth = Engine.QueryInterface(target, IID_Health);
	if (!cmpHealth)
		return;

	var targetState = cmpHealth.Increase(ApplyValueModificationsToEntity("Heal/HP", +this.template.HP, this.entity));

	// Add XP
	var cmpLoot = Engine.QueryInterface(target, IID_Loot);
	var cmpPromotion = Engine.QueryInterface(this.entity, IID_Promotion);
	if (targetState !== undefined && cmpLoot && cmpPromotion)
	{
		// HP healed * XP per HP
		cmpPromotion.IncreaseXp((targetState.new-targetState.old)*(cmpLoot.GetXp()/cmpHealth.GetMaxHitpoints()));
	}
	//TODO we need a sound file
//	PlaySound("heal_impact", this.entity);
};

Engine.RegisterComponentType(IID_Heal, "Heal", Heal);
