/**
 * All plan classes must implement this interface.
 */
var IPlan = Class({

	_init: function() { /* ... */ },

	canExecute: function(gameState) { /* ... */ },

	execute: function(gameState) { /* ... */ },

	getCost: function() { /* ... */ },
});

/**
 * Represents a prioritised collection of plans.
 */
var PlanGroup = Class({

	_init: function()
	{
		this.escrow = new Resources({});
		this.plans = [];
		this.resourcetime = 10 * 1000;
		this.resourcePlus = "food";
	},

	addPlan: function(priority, plan)
	{
		this.plans.push({"priority": priority, "plan": plan});
	},

	/**
	 * Executes all plans that we can afford, ordered by priority,
	 * and returns the highest-priority plan we couldn't afford (or null
	 * if none).
	 */
	executePlans: function(gameState, modules)
	{
		// Ignore impossible plans
		var plans = this.plans.filter(function(p) { return p.plan.canExecute(gameState); });

		// Sort by decreasing priority
		plans.sort(function(a, b) { return b.priority - a.priority; });

		// Execute as many plans as we can afford
		while (plans.length && this.escrow.canAfford(plans[0].plan.getCost()))
		{
			var plan = plans.shift().plan;
			this.escrow.subtract(plan.getCost());
			plan.execute(gameState);
		}

		if (plans.length){
		//if (gameState.getTimeElapsed() > this.resourcetime){
		var tempcheck = new Resources(this.escrow)
		for each (var pln in plans){
		tempcheck.subtract(pln.plan.getCost());
		}
		var squid = Object.keys(tempcheck);
		squid.sort(function(a, b) {
			return tempcheck[a] - tempcheck[b];
		});
		var str = "Resource check";
		for each (t in tempcheck.types){
		//JuBotAI.prototype.chat(str + " " + tempcheck[t] + " " + t);
		}
		//JuBotAI.prototype.chat(tempcheck.types.forEach(function(type) { str + " " + tempcheck[type] + " " + type }));
		//JuBotAI.prototype.chat("Resource order check " + squid[0] + " " + squid[1] + " " + squid[2] + " " + squid[3]);
		//EconomyManager.prototype.resourcePlus = squid[0];
		//JuBotAI.prototype.chat(EconomyManager.prototype.resourcePlus);
		//JuBotAI.prototype.chat(squid[0]);
		var str = squid[0];
		//warn(squid[0]);
		//JuBotAI.prototype.chat(str);
		modules[0].gatherWeights[squid[0]] = modules[0].gatherWeights[squid[0]] + 1;
		//warn(modules[0].gatherWeights[squid[0]]);
		//JuBotAI.prototype.chat(lolwut);
		//this.resourcetime = this.resourcetime + (60*1000);
		//}
			return plans[0];
			}
		else{
			return null;
			}
	},

	resetPlans: function()
	{
		this.plans = [];
	},

	getEscrow: function()
	{
		return this.escrow;
	},
});
