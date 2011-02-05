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
	executePlans: function(gameState)
	{
		// Ignore impossible plans
		var plans = this.plans.filter(function(p) { return p.plan.canExecute(gameState); });

		// Sort by decreasing priority
		plans.sort(function(a, b) { return a.priority > b.priority; });

		// Execute as many plans as we can afford
		while (plans.length && this.escrow.canAfford(plans[0].plan.getCost()))
		{
			var plan = plans.shift().plan;
			this.escrow.subtract(plan.getCost());
			plan.execute(gameState);
		}

		if (plans.length)
			return plans[0];
		else
			return null;
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
