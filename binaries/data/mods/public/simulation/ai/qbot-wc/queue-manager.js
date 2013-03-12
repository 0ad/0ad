//This takes the input queues and picks which items to fund with resources until no more resources are left to distribute.
//
//In this manager all resources are 'flattened' into a single type=(food+wood+metal+stone+pop*50 (see resources.js))
//the following refers to this simple as resource
//
// Each queue has an account which records the amount of resource it can spend.  If no queue has an affordable item
// then the amount of resource is increased to all accounts in direct proportion to the priority until an item on one
// of the queues becomes affordable.
//
// A consequence of the system is that a rarely used queue will end up with a very large account.  I am unsure if this
// is good or bad or neither.
//
// Each queue object has two queues in it, one with items waiting for resources and the other with items which have been 
// allocated resources and are due to be executed.  The secondary queues are helpful because then units can be trained
// in groups of 5 and buildings are built once per turn to avoid placement clashes.

var QueueManager = function(queues, priorities) {
	this.queues = queues;
	this.priorities = priorities;
	this.account = {};
	this.accounts = {};

	// the sorting would need to be updated on priority change but there is currently none.
	var self = this;
	this.queueArrays = [];
	for (var p in this.queues) {
		this.account[p] = 0;
		this.accounts[p] = new Resources();
		this.queueArrays.push([p,this.queues[p]]);
	}
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });

	this.curItemQueue = [];
	
};

QueueManager.prototype.getAvailableResources = function(gameState, noAccounts) {
	var resources = gameState.getResources();
	if (Config.difficulty == 1)
		resources.multiply(0.75);
	else if (Config.difficulty == 1)
		resources.multiply(0.5);
	if (noAccounts)
		return resources;
	for (var key in this.queues) {
		resources.subtract(this.accounts[key]);
	}
	return resources;
};

QueueManager.prototype.futureNeeds = function(gameState, EcoManager) {
	/*
	// Work out which plans will be executed next using priority and return the total cost of these plans
	var recurse = function(queues, qm, number, depth){
		var needs = new Resources();
		var totalPriority = 0;
		for (var i = 0; i < queues.length; i++){
			totalPriority += qm.priorities[queues[i]];
		}
		for (var i = 0; i < queues.length; i++){
			var num = Math.round(((qm.priorities[queues[i]]/totalPriority) * number));
			if (num < qm.queues[queues[i]].countQueuedUnits()){
				var cnt = 0;
				for ( var j = 0; cnt < num; j++) {
					cnt += qm.queues[queues[i]].queue[j].number;
					needs.add(qm.queues[queues[i]].queue[j].getCost());
					number -= qm.queues[queues[i]].queue[j].number;
				}
			}else{
				for ( var j = 0; j < qm.queues[queues[i]].length(); j++) {
					needs.add(qm.queues[queues[i]].queue[j].getCost());
					number -= qm.queues[queues[i]].queue[j].number;
				}
				queues.splice(i, 1);
				i--;
			}
		}
		// Check that more items were selected this call and that there are plans left to be allocated
		// Also there is a fail-safe max depth 
		if (queues.length > 0 && number > 0 && depth < 20){
			needs.add(recurse(queues, qm, number, depth + 1));
		}
		return needs;
	};
	
	//number of plans to look at
	var current = this.getAvailableResources(gameState, true);

	var futureNum = 20;
	var queues = [];
	for (var q in this.queues){
		queues.push(q);
	}
	var needs = recurse(queues, this, futureNum, 0);
	
	if (EcoManager === false) {
		return {
			"food" : Math.max(needs.food - current.food, 0),
			"wood" : Math.max(needs.wood + 15*needs.population - current.wood, 0),
			"stone" : Math.max(needs.stone - current.stone, 0),
			"metal" : Math.max(needs.metal - current.metal, 0)
		};
	} else {
		// Return predicted values minus the current stockpiles along with a base rater for all resources
		return {
			"food" : Math.max(needs.food - current.food, 0) + EcoManager.baseNeed["food"],
			"wood" : Math.max(needs.wood + 15*needs.population - current.wood, 0) + EcoManager.baseNeed["wood"], //TODO: read the house cost in case it changes in the future
			"stone" : Math.max(needs.stone - current.stone, 0) + EcoManager.baseNeed["stone"],
			"metal" : Math.max(needs.metal - current.metal, 0) + EcoManager.baseNeed["metal"]
		};
	}*/
	var needs = new Resources();
	// get ouy current resources, not removing accounts.
	var current = this.getAvailableResources(gameState, true);
	//queueArrays because it's faster.
	for (i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		for (var j = 0; j < Math.min(3,queue.length()); ++j)
		{
			needs.add(queue.queue[j].getCost());
		}
	}
	if (EcoManager === false) {
		return {
			"food" : Math.max(needs.food - current.food, 0),
			"wood" : Math.max(needs.wood - current.wood, 0),
			"stone" : Math.max(needs.stone - current.stone, 0),
			"metal" : Math.max(needs.metal - current.metal, 0)
		};
	} else {
		// Return predicted values minus the current stockpiles along with a base rater for all resources
		return {
			"food" : Math.max(needs.food - current.food, 0) + EcoManager.baseNeed["food"],
			"wood" : Math.max(needs.wood - current.wood, 0) + EcoManager.baseNeed["wood"],
			"stone" : Math.max(needs.stone - current.stone, 0) + EcoManager.baseNeed["stone"],
			"metal" : Math.max(needs.metal - current.metal, 0) + EcoManager.baseNeed["metal"]
		};
	}
};

QueueManager.prototype.printQueues = function(gameState){
	debug("OUTQUEUES");
	for (var i in this.queues){
		var qStr = "";
		var q = this.queues[i];
		if (q.outQueue.length > 0)
			debug((i + ":"));
		for (var j in q.outQueue){
			qStr = "	" + q.outQueue[j].type + " ";
			if (q.outQueue[j].number)
				qStr += "x" + q.outQueue[j].number;
			debug (qStr);
		}
	}
	
	debug("INQUEUES");
	for (var i in this.queues){
		var qStr = "";
		var q = this.queues[i];
		if (q.queue.length > 0)
			debug((i + ":"));
		for (var j in q.queue){
			qStr = "     " + q.queue[j].type + " ";
			if (q.queue[j].number)
				qStr += "x" + q.queue[j].number;
			debug (qStr);
		}
	}
	debug ("Accounts");
	for (p in this.accounts)
	{
		debug(p + ": " + uneval(this.accounts[p]));
	}
	debug("Needed Resources:" + uneval(this.futureNeeds(gameState,false)));
	debug ("Current Resources:" + uneval(gameState.getResources()));
	debug ("Available Resources:" + uneval(this.getAvailableResources(gameState)));
};

QueueManager.prototype.clear = function(){
	this.curItemQueue = [];
	for (i in this.queues)
		this.queues[i].empty();
};

QueueManager.prototype.update = function(gameState) {
	var self = this;
	
	for (var i in this.priorities){
		if (!(this.priorities[i] > 0)){
			this.priorities[i] = 1;  // TODO: make the Queue Manager not die when priorities are zero.
			warn("QueueManager received bad priorities, please report this error: " + uneval(this.priorities));
		}
	}
	
	Engine.ProfileStart("Queue Manager");
	
	//if (gameState.ai.playedTurn % 10 === 0)
	//	this.printQueues(gameState);
	
	Engine.ProfileStart("Pick items from queues");
	
	// TODO: this only pushes the first object. SHould probably try to push any possible object to maximize productivity. Perhaps a settinh?
	// looking at queues in decreasing priorities and pushing to the current item queues.
	for (i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.length() > 0)
		{
			var item = queue.getNext();
			var total = new Resources();
			total.add(this.accounts[name]);
			total.subtract(queue.outQueueCost());
			if (total.canAfford(item.getCost()))
			{
				queue.nextToOutQueue();
			}
		} else if (queue.totalLength() === 0) {
			this.accounts[name].reset();
		}
	}
	
	var availableRes = this.getAvailableResources(gameState);
	// assign some accounts to queues. This is done by priority, and by need. Note that this currently only looks at the next element.
	for (ress in availableRes)
	{
		if (availableRes[ress] > 0 && ress != "population")
		{
			var totalPriority = 0;
			// Okay so this is where it gets complicated.
			// If a queue requires "ress" for the next element (in the queue or the outqueue)
			// And the account is not high enough for it (multiplied by queue length... Might be bad, might not be).
			// Then we add it to the total priority.
			// (sorry about readability... Those big 'ifs' basically check if there is a need in the inqueue/outqueue
			for (j in this.queues) {
				if ((this.queues[j].length() > 0 && this.queues[j].getNext().getCost()[ress] > 0)
					|| (this.queues[j].outQueueLength() > 0 && this.queues[j].outQueueNext().getCost()[ress] > 0))
					if ( (this.queues[j].length() && this.accounts[j][ress] < this.queues[j].length() * (this.queues[j].getNext().getCost()[ress]))
						|| (this.queues[j].outQueueLength() && this.accounts[j][ress] < this.queues[j].outQueueLength() * (this.queues[j].outQueueNext().getCost()[ress])))
						totalPriority += this.priorities[j];
			}
			// Now we allow resources to the accounts. We can at most allow "priority/totalpriority*available"
			// But we'll sometimes allow less if that would overflow.
			for (j in this.queues) {
				if ((this.queues[j].length() > 0 && this.queues[j].getNext().getCost()[ress] > 0)
					|| (this.queues[j].outQueueLength() > 0 && this.queues[j].outQueueNext().getCost()[ress] > 0))
				{
					// we'll add at much what can be allowed to this queue.
					var toAdd = Math.floor(this.priorities[j]/totalPriority * availableRes[ress]);
					
					var maxNeed = 0;
					if (this.queues[j].length())
						for (var y = 0; y < Math.min(3,this.queues[j].length()); ++y)
							maxNeed += this.queues[j].queue[y].getCost()[ress];
					if (this.queues[j].outQueueLength())
						for (var y = 0; y < this.queues[j].outQueueLength(); ++y)
							maxNeed += this.queues[j].outQueue[y].getCost()[ress];
					if (toAdd + this.accounts[j][ress] > maxNeed)
						toAdd = maxNeed - this.accounts[j][ress];	// always inferior to the original level.
					//debug ("Adding " + toAdd + " of " + ress + " to the account of " + j);
					this.accounts[j][ress] += toAdd;
				}
			}
		}
	}
	Engine.ProfileStop();

	Engine.ProfileStart("Execute items");
	
	var units_Techs_passed = 0;
	// Handle output queues by executing items where possible
	for (var p in this.queueArrays) {
		var name = this.queueArrays[p][0];
		var queue = this.queueArrays[p][1];
		while (queue.outQueueLength() > 0) {
			var next = queue.outQueueNext();
			if (next.category === "building") {
				if (gameState.buildingsBuilt == 0) {
					if (next.canExecute(gameState)) {
						this.accounts[name].subtract(next.getCost())
						//debug ("Starting " + next.type + " substracted " + uneval(next.getCost()))
						queue.executeNext(gameState);
						gameState.buildingsBuilt += 1;
					} else {
						break;
					}
				} else {
					break;
				}
			} else {
				if (units_Techs_passed < 2 && queue.outQueueNext().canExecute(gameState)){
					//debug ("Starting " + next.type + " substracted " + uneval(next.getCost()))
					this.accounts[name].subtract(next.getCost())
					queue.executeNext(gameState);
					units_Techs_passed++;
				} else {
					break;
				}
			}
		}
		if (units_Techs_passed >= 2)
			break;
	}
	Engine.ProfileStop();
	Engine.ProfileStop();
};

QueueManager.prototype.addQueue = function(queueName, priority) {
	if (this.queues[queueName] == undefined) {
		this.queues[queueName] = new Queue();
		this.priorities[queueName] = priority;
		this.account[queueName] = 0;
		this.accounts[queueName] = new Resources();

		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
}
QueueManager.prototype.removeQueue = function(queueName) {
	if (this.queues[queueName] !== undefined) {
		if ( this.curItemQueue.indexOf(queueName) !== -1) {
			this.curItemQueue.splice(this.curItemQueue.indexOf(queueName),1);
		}
		delete this.queues[queueName];
		delete this.priorities[queueName];
		delete this.account[queueName];
		delete this.accounts[queueName];
		
		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
}
QueueManager.prototype.changePriority = function(queueName, newPriority) {
	var self = this;
	if (this.queues[queueName] !== undefined)
		this.priorities[queueName] = newPriority;
	this.queueArrays = [];
	for (var p in this.queues) {
		this.account[p] = 0;
		this.accounts[p] = new Resources();
		this.queueArrays.push([p,this.queues[p]]);
	}
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
}

