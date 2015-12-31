function Repairable() {}

Repairable.prototype.Schema =
	"<a:help>Deals with repairable structures and units.</a:help>" +
	"<a:example>" +
		"<RepairTimeRatio>2.0</RepairTimeRatio>" +
	"</a:example>" +
	"<element name='RepairTimeRatio' a:help='repair time ratio relative to building (or production) time.'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

Repairable.prototype.Init = function()
{
	this.builders = []; // builder entities
	this.buildMultiplier = 1; // Multiplier for the amount of work builders do.
	this.repairTimeRatio = +this.template.RepairTimeRatio;
};

Repairable.prototype.GetNumBuilders = function()
{
	return this.builders.length;
};

Repairable.prototype.AddBuilder = function(builderEnt)
{
	if (this.builders.indexOf(builderEnt) !== -1)
		return;
	this.builders.push(builderEnt);
	this.SetBuildMultiplier();
};

Repairable.prototype.RemoveBuilder = function(builderEnt)
{
	if (this.builders.indexOf(builderEnt) === -1)
		return;
	this.builders.splice(this.builders.indexOf(builderEnt), 1);
 	this.SetBuildMultiplier();
};

/**
 * Sets the build rate multiplier, which is applied to all builders.
 * Yields a total rate of construction equal to numBuilders^0.7
 */
Repairable.prototype.SetBuildMultiplier = function()
{
	let numBuilders = this.builders.length;
	if (numBuilders < 2)
		this.buildMultiplier = 1;
	else
		this.buildMultiplier = Math.pow(numBuilders, 0.7) / numBuilders;
};

// TODO: should we have resource costs?
Repairable.prototype.Repair = function(builderEnt, rate)
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	if (!cmpHealth || !cmpCost)
		return;
	let damage = cmpHealth.GetMaxHitpoints() - cmpHealth.GetHitpoints();
	if (damage <= 0)
		return;

	// Calculate the amount of hitpoints that will be added (using diminishing rate when several builders)
	let work = rate * this.buildMultiplier * this.GetRepairRate();
	let amount = Math.min(damage, work);
	cmpHealth.Increase(amount);

	// If we repaired all the damage, send a message to entities to stop repairing this building
	if (amount >= damage)
		Engine.PostMessage(this.entity, MT_ConstructionFinished, { "entity": this.entity, "newentity": this.entity });
};

Repairable.prototype.GetRepairRate = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	let repairTime = this.repairTimeRatio * cmpCost.GetBuildTime();
	return repairTime ? cmpHealth.GetMaxHitpoints() / repairTime : 1;
};

Engine.RegisterComponentType(IID_Repairable, "Repairable", Repairable);
