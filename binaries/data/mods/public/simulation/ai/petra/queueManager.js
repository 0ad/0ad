var PETRA = function(m)
{

// This takes the input queues and picks which items to fund with resources until no more resources are left to distribute.
//
// Currently this manager keeps accounts for each queue, split between the 4 main resources
//
// Each time resources are available (ie not in any account), it is split between the different queues
// Mostly based on priority of the queue, and existing needs.
// Each turn, the queue Manager checks if a queue can afford its next item, then it does.
//
// A consequence of the system it's not really revertible. Once a queue has an account of 500 food, it'll keep it
// If for some reason the AI stops getting new food, and this queue lacks, say, wood, no other queues will
// be able to benefit form the 500 food (even if they only needed food).
// This is not to annoying as long as all goes well. If the AI loses many workers, it starts being problematic.
//
// It also has the effect of making the AI more or less always sit on a few hundreds resources since most queues
// get some part of the total, and if all queues have 70% of their needs, nothing gets done
// Particularly noticeable when phasing: the AI often overshoots by a good 200/300 resources before starting.
//
// This system should be improved. It's probably not flexible enough.

m.QueueManager = function(Config, queues)
{
	this.Config = Config;
	this.queues = queues;
	this.priorities = {};
	for (var i in Config.priorities)
		this.priorities[i] = Config.priorities[i];
	this.accounts = {};

	// the sorting is updated on priority change.
	var self = this;
	this.queueArrays = [];
	for (var p in this.queues)
	{
		this.accounts[p] = new API3.Resources();
		this.queueArrays.push([p,this.queues[p]]);
	}
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
};

m.QueueManager.prototype.getAvailableResources = function(gameState)
{
	var resources = gameState.getResources();
	for (var key in this.queues)
		resources.subtract(this.accounts[key]);
	return resources;
};

m.QueueManager.prototype.getTotalAccountedResources = function(gameState)
{
	var resources = new API3.Resources();
	for (var key in this.queues)
		resources.add(this.accounts[key]);
	return resources;
};

m.QueueManager.prototype.currentNeeds = function(gameState)
{
	var needed = new API3.Resources();
	//queueArrays because it's faster.
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.length() == 0 || !queue.queue[0].isGo(gameState))
			continue;
		var costs = queue.queue[0].getCost();
		needed.add(costs);
	}
	// get out current resources, not removing accounts.
	var current = gameState.getResources();
	for (var res of needed.types)
		needed[res] = Math.max(0, needed[res] - current[res]);

	return needed;
};

// calculate the gather rates we'd want to be able to start all elements in our queues
// TODO: many things.
m.QueueManager.prototype.wantedGatherRates = function(gameState)
{
	// get out current resources, not removing accounts.
	var current = gameState.getResources();
	// short queue is the first item of a queue, assumed to be ready in 30s
	// medium queue is the second item of a queue, assumed to be ready in 60s
	// long queue contains the is the isGo=false items, assumed to be ready in 300s
	var totalShort = { "food": 200, "wood": 200, "stone": 100, "metal": 100 };
	var totalMedium = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	var totalLong = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	var total;
	//queueArrays because it's faster.
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.paused)
			continue;
		for (var j = 0; j < queue.length(); ++j)
		{
			if (j > 1)
				break;
			var cost = queue.queue[j].getCost();
			if (queue.queue[j].isGo(gameState))
			{
				if (j == 0)
					total = totalShort;
				else
					total = totalMedium;
			}
			else
				total = totalLong;
			for (var type in total)
				total[type] += cost[type];
			if (!queue.queue[j].isGo(gameState))
				break;
		}
	}
	// global rates
	var rates = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	var diff;
	for (var type in rates)
	{
		if (current[type] > 0)
		{
			diff = Math.min(current[type], totalShort[type]);
			totalShort[type] -= diff;
			current[type] -= diff;
			if (current[type] > 0)
			{
				diff = Math.min(current[type], totalMedium[type]);
				totalMedium[type] -= diff;
				current[type] -= diff;
				if (current[type] > 0)
					totalLong[type] -= Math.min(current[type], totalLong[type]);
			}
		}
		rates[type] = totalShort[type]/30 + totalMedium[type]/60 + totalLong[type]/300;
	}

	return rates;
};

m.QueueManager.prototype.printQueues = function(gameState)
{
	var numWorkers = 0;
	gameState.getOwnUnits().forEach (function (ent) {
		if (ent.getMetadata(PlayerID, "role") == "worker" && ent.getMetadata(PlayerID, "plan") == undefined)
			numWorkers++;
	});
	API3.warn("---------- QUEUES ------------ with pop " + gameState.getPopulation() + " and workers " + numWorkers);
	for (var i in this.queues)
	{
		var qStr = "";
		var q = this.queues[i];
		if (q.queue.length > 0)
		{
			API3.warn(i + ": ( with priority " + this.priorities[i] +" and accounts " + uneval(this.accounts[i]) +")");
			API3.warn(" while maxAccountWanted(0.6) is " + uneval(q.maxAccountWanted(gameState, 0.6)));
		}
		for (var j in q.queue)
		{
			qStr = "     " + q.queue[j].type + " ";
			if (q.queue[j].number)
				qStr += "x" + q.queue[j].number;
			qStr += "   isGo " + q.queue[j].isGo(gameState);
			API3.warn(qStr);
		}
	}
	API3.warn("Accounts");
	for (var p in this.accounts)
	    API3.warn(p + ": " + uneval(this.accounts[p]));
	API3.warn("Current Resources: " + uneval(gameState.getResources()));
	API3.warn("Available Resources: " + uneval(this.getAvailableResources(gameState)));
	API3.warn("Wanted Gather Rates: " + uneval(this.wantedGatherRates(gameState)));
	API3.warn("Current Gather Rates: " + uneval(gameState.ai.HQ.GetCurrentGatherRates(gameState)));
	API3.warn("Most needed resources: " + uneval(gameState.ai.HQ.pickMostNeededResources(gameState)));
	API3.warn("------------------------------------");
};

// nice readable HTML version.
m.QueueManager.prototype.HTMLprintQueues = function(gameState)
{
	if (!m.DebugEnabled())
		return;
	var strToSend = [];
	strToSend.push("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\"> <html> <head> <title>Aegis Queue Manager</title> <link rel=\"stylesheet\" href=\"table.css\">  </head> <body> <table> <caption>Aegis Build Order</caption> ");
	for (var i in this.queues)
	{
		strToSend.push("<tr>");
		
		var q = this.queues[i];
		var str = "<th>" + i + "  (" + this.priorities[i] + ")<br><span class=\"ressLevel\">";
		for (var k of this.accounts[i].types)
		{
			str += this.accounts[i][k] + k.substr(0,1).toUpperCase() ;
			if (k != "metal") str += " / ";
		}
		strToSend.push(str + "</span></th>");
		for (var j in q.queue) {
			if (q.queue[j].isGo(gameState))
				strToSend.push("<td>");
			else
				strToSend.push("<td class=\"NotGo\">");

			var qStr = "";
			if (q.queue[j].number)
				qStr += q.queue[j].number + " ";
			qStr += q.queue[j].type;
			qStr += "<br><span class=\"ressLevel\">";
			var costs = q.queue[j].getCost();
			for (var k of costs.types)
			{
				qStr += costs[k] + k.substr(0,1).toUpperCase() ;
				if (k != "metal")
					qStr += " / ";
			}
			qStr += "</span></td>";
			strToSend.push(qStr);
		}
		strToSend.push("</tr>");
	}
	strToSend.push("</table>");
	/*strToSend.push("<h3>Accounts</h3>");
	for (var p in this.accounts)
	{
		strToSend.push("<p>" + p + ": " + uneval(this.accounts[p]) + " </p>");
	}*/
	strToSend.push("<p>Wanted Gather Rate:" + uneval(this.wantedGatherRates(gameState)) + "</p>");
	strToSend.push("<p>Current Resources:" + uneval(gameState.getResources()) + "</p>");
	strToSend.push("<p>Available Resources:" + uneval(this.getAvailableResources(gameState)) + "</p>");
	strToSend.push("</body></html>");
	for (var logged of strToSend)
		log(logged);
};

m.QueueManager.prototype.clear = function()
{
	for (var i in this.queues)
		this.queues[i].empty();
};

/**
 * transfer accounts from queue i to queue j
 */
m.QueueManager.prototype.transferAccounts = function(cost, i, j)
{
	for (var res of this.accounts[i].types)
	{
		if (this.accounts[j][res] >= cost[res])
			continue;
		var diff = Math.min(this.accounts[i][res], cost[res] - this.accounts[j][res]);
		this.accounts[i][res] -= diff;
		this.accounts[j][res] += diff;
	}
};

/**
 * distribute the resources between the different queues according to their priorities
 */
m.QueueManager.prototype.distributeResources = function(gameState)
{
	var availableRes = this.getAvailableResources(gameState);
	for (var res of availableRes.types)
	{
		if (availableRes[res] < 0)    // rescale the accounts if we've spent resources already accounted (e.g. by bartering)
		{
			var total = gameState.getResources()[res];
			var scale = total / (total - availableRes[res]);
			availableRes[res] = total;
			for (var j in this.queues)
			{
				this.accounts[j][res] = Math.floor(scale * this.accounts[j][res]);
				availableRes[res] -= this.accounts[j][res];
			}
		}

		if (!availableRes[res])
		{
			this.switchResource(gameState, res);
			continue;
		}

		var totalPriority = 0;
		var tempPrio = {};
		var maxNeed = {};
		// Okay so this is where it gets complicated.
		// If a queue requires "res" for the next elements (in the queue)
		// And the account is not high enough for it.
		// Then we add it to the total priority.
		// To try and be clever, we don't want a long queue to hog all resources. So two things:
		//	-if a queue has enough of resource X for the 1st element, its priority is decreased (factor 2).
		//	-queues accounts are capped at "resources for the first + 60% of the next"
		// This avoids getting a high priority queue with many elements hogging all of one resource
		// uselessly while it awaits for other resources.
		for (var j in this.queues)
		{
			// returns exactly the correct amount, ie 0 if we're not go.
			var queueCost = this.queues[j].maxAccountWanted(gameState, 0.6);
			if (this.queues[j].length() > 0 && this.accounts[j][res] < queueCost[res] && !this.queues[j].paused)
			{
				// adding us to the list of queues that need an update.
				tempPrio[j] = this.priorities[j];
				maxNeed[j] = queueCost[res] - this.accounts[j][res];
				// if we have enough of that resource for our first item in the queue, diminish our priority.
				if (this.accounts[j][res] >= this.queues[j].getNext().getCost()[res])
					tempPrio[j] /= 2;

				if (tempPrio[j])
					totalPriority += tempPrio[j];
			}
			else if (this.accounts[j][res] > queueCost[res])
			{
				availableRes[res] += (this.accounts[j][res] - queueCost[res]);
				this.accounts[j][res] = queueCost[res];
			}
		}
		// Now we allow resources to the accounts. We can at most allow "TempPriority/totalpriority*available"
		// But we'll sometimes allow less if that would overflow.
		var available = availableRes[res];
		var missing = false;
		for (var j in tempPrio)
		{
			// we'll add at much what can be allowed to this queue.
			var toAdd = Math.floor(availableRes[res] * tempPrio[j]/totalPriority);
			if (toAdd >= maxNeed[j])
				toAdd = maxNeed[j];
			else
				missing = true;
			this.accounts[j][res] += toAdd;
			maxNeed[j] -= toAdd;
			available -= toAdd;
		}
		if (missing && available > 0)   // distribute the rest (due to floor) in any queue
		{
			for (var j in tempPrio)
			{
				var toAdd = Math.min(maxNeed[j], available);
				this.accounts[j][res] += toAdd;
				available -= toAdd;
				if (available <= 0)
					break;
			}
		}
		if (available < 0)
			API3.warn("Petra: problem with remaining " + res + " in queueManager " + available);
	}
};

m.QueueManager.prototype.switchResource = function(gameState, res)
{
	// We have no available resources, see if we can't "compact" them in one queue.
	// compare queues 2 by 2, and if one with a higher priority could be completed by our amount, give it.
	// TODO: this isn't perfect compression.
	for (var j in this.queues)
	{
		if (this.queues[j].length() === 0 || this.queues[j].paused)
			continue;

		var queue = this.queues[j];
		var queueCost = queue.maxAccountWanted(gameState, 0);
		if (this.accounts[j][res] >= queueCost[res])
			continue;

		for (var i in this.queues)
		{
			if (i === j)
				continue;
			var otherQueue = this.queues[i];
			if (this.priorities[i] >= this.priorities[j] || otherQueue.switched !== 0)
				continue;
			if (this.accounts[j][res] + this.accounts[i][res] < queueCost[res])
				continue;

			var diff = queueCost[res] - this.accounts[j][res];
			this.accounts[j][res] += diff;
			this.accounts[i][res] -= diff;
			++otherQueue.switched;
			if (this.Config.debug > 1)
				API3.warn ("switching queue " + res + " from " + i + " to " + j + " in amount " + diff);
			break;
		}
	}
};

// Start the next item in the queue if we can afford it.
m.QueueManager.prototype.startNextItems = function(gameState)
{
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.length() > 0 && !queue.paused)
		{
			var item = queue.getNext();
			if (this.accounts[name].canAfford(item.getCost()) && item.canStart(gameState))
			{
				this.finishingTime = gameState.ai.elapsedTime;
				this.accounts[name].subtract(item.getCost());
				queue.startNext(gameState);
				queue.switched = 0;
			}
		}
		else if (queue.length() === 0)
		{
			this.accounts[name].reset();
			queue.switched = 0;
		}
	}
};

m.QueueManager.prototype.update = function(gameState)
{
	Engine.ProfileStart("Queue Manager");

	for (var i in this.queues)
	{
		this.queues[i].check(gameState);  // do basic sanity checks on the queue
		if (this.priorities[i] > 0)
			continue;
		API3.warn("QueueManager received bad priorities, please report this error: " + uneval(this.priorities));
		this.priorities[i] = 1;  // TODO: make the Queue Manager not die when priorities are zero.
	}

	// Let's assign resources to plans that need them
	this.distributeResources(gameState);

	// Start the next item in the queue if we can afford it.
	this.startNextItems(gameState);

	if (this.Config.debug > 0 && gameState.ai.playedTurn%50 === 0)
		this.printQueues(gameState);
	
	Engine.ProfileStop();
};

m.QueueManager.prototype.pauseQueue = function(queue, scrapAccounts)
{
	if (this.queues[queue])
	{
		this.queues[queue].paused = true;
		if (scrapAccounts)
			this.accounts[queue].reset();
	}
};

m.QueueManager.prototype.unpauseQueue = function(queue)
{
	if (this.queues[queue])
		this.queues[queue].paused = false;
};

m.QueueManager.prototype.pauseAll = function(scrapAccounts, but)
{
	for (var p in this.queues)
	{
		if (p != but)
		{
			if (scrapAccounts)
				this.accounts[p].reset();
			this.queues[p].paused = true;
		}
	}
};

m.QueueManager.prototype.unpauseAll = function(but)
{
	for (var p in this.queues)
		if (p != but)
			this.queues[p].paused = false;
};


m.QueueManager.prototype.addQueue = function(queueName, priority)
{
	if (this.queues[queueName] == undefined)
	{
		this.queues[queueName] = new m.Queue();
		this.priorities[queueName] = priority;
		this.accounts[queueName] = new API3.Resources();

		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
};

m.QueueManager.prototype.removeQueue = function(queueName)
{
	if (this.queues[queueName] !== undefined)
	{
		delete this.queues[queueName];
		delete this.priorities[queueName];
		delete this.accounts[queueName];
		
		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
};

m.QueueManager.prototype.getPriority = function(queueName)
{
	return this.priorities[queueName];
};

m.QueueManager.prototype.changePriority = function(queueName, newPriority)
{
	if (this.Config.debug > 0)
		API3.warn(">>> Priority of queue " + queueName + " changed from " + this.priorities[queueName] + " to " + newPriority);
	var self = this;
	if (this.queues[queueName] !== undefined)
		this.priorities[queueName] = newPriority;
	this.queueArrays = [];
	for (var p in this.queues)
		this.queueArrays.push([p,this.queues[p]]);
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
};

return m;
}(PETRA);
