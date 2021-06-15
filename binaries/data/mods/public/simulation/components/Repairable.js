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
	this.builders = new Map(); // Map of builder entities to their work per second
	this.totalBuilderRate = 0; // Total amount of work the builders do each second
	this.buildMultiplier = 1; // Multiplier for the amount of work builders do
	this.buildTimePenalty = 0.7; // Penalty for having multiple builders
	this.repairTimeRatio = +this.template.RepairTimeRatio;
};

/**
 * Returns the current build progress in a [0,1] range.
 */
Repairable.prototype.GetBuildProgress = function()
{
	var cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (!cmpHealth)
		return 0;

	var hitpoints = cmpHealth.GetHitpoints();
	var maxHitpoints = cmpHealth.GetMaxHitpoints();

	return hitpoints / maxHitpoints;
};

/**
 * @return whether this entity can be repaired (this does not account for health).
 */
Repairable.prototype.IsRepairable = function()
{
	return !this.unrepairable;
};

Repairable.prototype.SetRepairability = function(repairable)
{
	this.unrepairable = !repairable;
};

/**
 * Returns the current builders.
 *
 * @return {number[]} - An array containing the entity IDs of assigned builders.
 */
Repairable.prototype.GetBuilders = function()
{
	return Array.from(this.builders.keys());
};

Repairable.prototype.GetNumBuilders = function()
{
	return this.builders.size;
};

/**
 * Adds an array of builders.
 *
 * @param {number[]} - An array containing the entity IDs of builders to assign.
 */
Repairable.prototype.AddBuilders = function(builders)
{
	for (let builder of builders)
		this.AddBuilder(builder);
};

Repairable.prototype.AddBuilder = function(builderEnt)
{
	if (this.builders.has(builderEnt))
		return;

	this.builders.set(builderEnt, Engine.QueryInterface(builderEnt, IID_Builder).GetRate());
	this.totalBuilderRate += this.builders.get(builderEnt);
	this.SetBuildMultiplier();
};

Repairable.prototype.RemoveBuilder = function(builderEnt)
{
	if (!this.builders.has(builderEnt))
		return;

	this.totalBuilderRate -= this.builders.get(builderEnt);
	this.builders.delete(builderEnt);
	this.SetBuildMultiplier();
};

/**
 * The build multiplier is a penalty that is applied to each builder.
 * For example, ten women build at a combined rate of 10^0.7 = 5.01 instead of 10.
 */
Repairable.prototype.CalculateBuildMultiplier = function(num)
{
	// Avoid division by zero, in particular 0/0 = NaN which isn't reliably serialized
	return num < 2 ? 1 : Math.pow(num, this.buildTimePenalty) / num;
};

Repairable.prototype.SetBuildMultiplier = function()
{
	this.buildMultiplier = this.CalculateBuildMultiplier(this.GetNumBuilders());
};

Repairable.prototype.GetBuildTime = function()
{
	let timeLeft = (1 - this.GetBuildProgress()) * Engine.QueryInterface(this.entity, IID_Cost).GetBuildTime() * this.repairTimeRatio;
	let rate = this.totalBuilderRate * this.buildMultiplier;
	// The rate if we add another woman to the repairs
	let rateNew = (this.totalBuilderRate + 1) * this.CalculateBuildMultiplier(this.GetNumBuilders() + 1);
	return {
		// Avoid division by zero, in particular 0/0 = NaN which isn't reliably serialized
		"timeRemaining": rate ? timeLeft / rate : 0,
		"timeRemainingNew": timeLeft / rateNew
	};
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

	// Update the total builder rate
	this.totalBuilderRate += rate - this.builders.get(builderEnt);
	this.builders.set(builderEnt, rate);

	// If we repaired all the damage, send a message to entities to stop repairing this building
	if (amount >= damage)
	{
		Engine.PostMessage(this.entity, MT_ConstructionFinished, { "entity": this.entity, "newentity": this.entity });

		// Inform the builders that repairing has finished.
		// This not done by listening to a global message due to performance.
		for (let builder of this.GetBuilders())
		{
			let cmpUnitAIBuilder = Engine.QueryInterface(builder, IID_UnitAI);
			if (cmpUnitAIBuilder)
				cmpUnitAIBuilder.ConstructionFinished({ "entity": this.entity, "newentity": this.entity });
		}
	}
};

Repairable.prototype.GetRepairRate = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	let cmpCost = Engine.QueryInterface(this.entity, IID_Cost);
	let repairTime = this.repairTimeRatio * cmpCost.GetBuildTime();
	return repairTime ? cmpHealth.GetMaxHitpoints() / repairTime : 1;
};

Repairable.prototype.OnEntityRenamed = function(msg)
{
	let cmpRepairableNew = Engine.QueryInterface(msg.newentity, IID_Repairable);
	if (cmpRepairableNew)
		cmpRepairableNew.AddBuilders(this.GetBuilders());
};

function RepairableMirage() {}
RepairableMirage.prototype.Init = function(cmpRepairable)
{
	this.numBuilders = cmpRepairable.GetNumBuilders();
	this.buildTime = cmpRepairable.GetBuildTime();
	if (cmpRepairable.unrepairable)
		this.unrepairable = cmpRepairable.unrepairable;
};

RepairableMirage.prototype.GetNumBuilders = function() { return this.numBuilders; };
RepairableMirage.prototype.GetBuildTime = function() { return this.buildTime; };
RepairableMirage.prototype.IsRepairable = function() { return !this.unrepairable; };

Engine.RegisterGlobal("RepairableMirage", RepairableMirage);

Repairable.prototype.Mirage = function()
{
	let mirage = new RepairableMirage();
	mirage.Init(this);
	return mirage;
};

Engine.RegisterComponentType(IID_Repairable, "Repairable", Repairable);
