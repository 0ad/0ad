/*
 * This is a primitive initial attempt at an AI player.
 * The design isn't great and maybe the whole thing should be rewritten -
 * the aim here is just to have something that basically works, and to
 * learn more about what's really needed for a decent AI design.
 *
 * The basic idea is we have a collection of independent modules
 * (EconomyManager, etc) which produce a list of plans.
 * The modules are mostly stateless - each turn they look at the current
 * world state, and produce some plans that will improve the state.
 * E.g. if there's too few worker units, they'll do a plan to train
 * another one. Plans are discarded after the current turn, if they
 * haven't been executed.
 *
 * Plans are grouped into a small number of PlanGroups, and for each
 * group we try to execute the highest-priority plans.
 * If some plan groups need more resources to execute their highest-priority
 * plan, we'll distribute any unallocated resources to that group's
 * escrow account. Eventually they'll accumulate enough to afford their plan.
 * (The purpose is to ensure resources are shared fairly between all the
 * plan groups - none of them should be starved even if they're trying to
 * execute a really expensive plan.)
 */

/*
 * Lots of things we should fix:
 *
 *  * Find entities with no assigned role, and give them something to do
 *  * Keep some units back for defence
 *  * Consistent terminology (type vs template etc)
 *  * ...
 *
 */


function TestBotAI(settings)
{
//	warn("Constructing TestBotAI for player "+settings.player);

	BaseAI.call(this, settings);

	this.turn = 0;

	this.modules = [
		new EconomyManager(),
		new MilitaryAttackManager(),
	];

	this.planGroups = {
		economyPersonnel: new PlanGroup(),
		economyConstruction: new PlanGroup(),
		militaryPersonnel: new PlanGroup(),
	};
}

TestBotAI.prototype = new BaseAI();

TestBotAI.prototype.ShareResources = function(remainingResources, unaffordablePlans)
{
	// Share our remaining resources among the plangroups that need
	// to accumulate more resources, in proportion to their priorities

	for each (var type in remainingResources.types)
	{
		// Skip resource types where we don't have any spare
		if (remainingResources[type] <= 0)
			continue;

		// Find the plans that require some of this resource type,
		// and the sum of their priorities
		var ps = [];
		var sumPriority = 0;
		for each (var p in unaffordablePlans)
		{
			if (p.plan.getCost()[type] > p.group.getEscrow()[type])
			{
				ps.push(p);
				sumPriority += p.priority;
			}
		}

		// Avoid divisions-by-zero
		if (!sumPriority)
			continue;

		// Share resources by priority, clamped to the amount the plan actually needs
		for each (var p in ps)
		{
			var amount = Math.floor(remainingResources[type] * p.priority / sumPriority);
			var max = p.plan.getCost()[type] - p.group.getEscrow()[type];
			p.group.getEscrow()[type] += Math.min(max, amount);
		}
	}
};

TestBotAI.prototype.OnUpdate = function()
{
	// Run the update every n turns, offset depending on player ID to balance the load
	if ((this.turn + this.player) % 4 == 0)
	{
		var gameState = new GameState(this);

		// Find the resources we have this turn that haven't already
		// been allocated to an escrow account.
		// (We need to do this before executing any plans, because those will
		// distort the escrow figures.)
		var remainingResources = gameState.getResources();
		for each (var planGroup in this.planGroups)
			remainingResources.subtract(planGroup.getEscrow());

		Engine.ProfileStart("plan setup");

		// Compute plans from each module
		for each (var module in this.modules)
			module.update(gameState, this.planGroups);

//		print(uneval(this.planGroups)+"\n");

		Engine.ProfileStop();
		Engine.ProfileStart("plan execute");

		// Execute as many plans as possible, and keep a record of
		// which ones we can't afford yet
		var unaffordablePlans = [];
		for each (var planGroup in this.planGroups)
		{
			var plan = planGroup.executePlans(gameState);
			if (plan)
				unaffordablePlans.push({"group": planGroup, "priority": plan.priority, "plan": plan.plan});
		}

		Engine.ProfileStop();

		this.ShareResources(remainingResources, unaffordablePlans);

//		print(uneval(this.planGroups)+"\n");

		// Reset the temporary plan data
		for each (var planGroup in this.planGroups)
			planGroup.resetPlans();
	}

	this.turn++;
};
