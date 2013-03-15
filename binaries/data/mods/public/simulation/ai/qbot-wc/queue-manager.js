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
	if (noAccounts)
		return resources;
	for (var key in this.queues) {
		resources.subtract(this.accounts[key]);
	}
	return resources;
};

QueueManager.prototype.futureNeeds = function(gameState, EcoManager) {
	var needs = new Resources();
	// get ouy current resources, not removing accounts.
	var current = this.getAvailableResources(gameState, true);
	//queueArrays because it's faster.
	for (i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		for (var j = 0; j < Math.min(2,queue.length()); ++j)
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
			"food" : (Math.max(needs.food - current.food, 0) + EcoManager.baseNeed["food"])/2,
			"wood" : (Math.max(needs.wood - current.wood, 0) + EcoManager.baseNeed["wood"])/2,
			"stone" : (Math.max(needs.stone - current.stone, 0) + EcoManager.baseNeed["stone"])/2,
			"metal" : (Math.max(needs.metal - current.metal, 0) + EcoManager.baseNeed["metal"])/2
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
	for (var i in this.queueArrays)
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
	// assign some accounts to queues. This is done by priority, and by need.
	for (ress in availableRes)
	{
		if (availableRes[ress] > 0 && ress != "population")
		{
			var totalPriority = 0;
			var tempPrio = {};
			var maxNeed = {};
			// Okay so this is where it gets complicated.
			// If a queue requires "ress" for the next elements (in the queue or the outqueue)
			// And the account is not high enough for it.
			// Then we add it to the total priority.
			// To try and be clever, we don't want a long queue to hog all resources. So two things:
			//	-if a queue has enough of resource X for the 1st element, its priority is decreased (/2).
			//	-queues accounts are capped at "resources for the first + 80% of the next"
			// This avoids getting a high priority queue with many elements hogging all of one resource
			// uselessly while it awaits for other resources.
			for (j in this.queues) {
				var outQueueCost = this.queues[j].outQueueCost();
				var queueCost = this.queues[j].queueCost();
				if (this.accounts[j][ress] < queueCost[ress] + outQueueCost[ress])
				{
					// adding us to the list of queues that need an update.
					tempPrio[j] = this.priorities[j];
					maxNeed[j] = outQueueCost[ress] + this.queues[j].getNext().getCost()[ress];
					// if we have enough of that resource for the outqueue and our first resource in the queue, diminish our priority.
					if (this.accounts[j][ress] >= outQueueCost[ress] + this.queues[j].getNext().getCost()[ress])
					{
						tempPrio[j] /= 2;
						if (this.queues[j].length() !== 1)
						{
							var halfcost = this.queues[j].queue[1].getCost()[ress]*0.8;
							maxNeed[j] += halfcost;
							if (this.accounts[j][ress] >= outQueueCost[ress] + this.queues[j].getNext().getCost()[ress] + halfcost)
								delete tempPrio[j];
						}
					}
					if (tempPrio[j])
						totalPriority += tempPrio[j];
				}
			}
			// Now we allow resources to the accounts. We can at most allow "TempPriority/totalpriority*available"
			// But we'll sometimes allow less if that would overflow.
			for (j in tempPrio) {
				// we'll add at much what can be allowed to this queue.
				var toAdd = Math.floor(tempPrio[j]/totalPriority * availableRes[ress]);
				// let's check we're not adding too much.
				var maxAdd = Math.min(maxNeed[j] - this.accounts[j][ress], toAdd);
				this.accounts[j][ress] += maxAdd;
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
		var next = queue.outQueueNext();
		if (!next)
			continue;
		if (next.category === "building") {
			if (gameState.buildingsBuilt == 0) {
				if (next.canExecute(gameState)) {
					this.accounts[name].subtract(next.getCost())
					//debug ("Starting " + next.type + " substracted " + uneval(next.getCost()))
					queue.executeNext(gameState);
					gameState.buildingsBuilt += 1;
				}
			}
		} else {
			if (units_Techs_passed < 2 && queue.outQueueNext().canExecute(gameState)){
				//debug ("Starting " + next.type + " substracted " + uneval(next.getCost()))
				this.accounts[name].subtract(next.getCost())
				queue.executeNext(gameState);
				units_Techs_passed++;
			}
		}
		if (units_Techs_passed >= 2)
			continue;
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
		this.queueArrays.push([p,this.queues[p]]);
	}
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
}

