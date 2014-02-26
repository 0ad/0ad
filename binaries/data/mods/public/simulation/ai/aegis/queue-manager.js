var AEGIS = function(m)
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

m.QueueManager = function(Config, queues, priorities) {
	this.Config = Config;
	this.queues = queues;
	this.priorities = priorities;
	this.accounts = {};

	// the sorting is updated on priority change.
	var self = this;
	this.queueArrays = [];
	for (var p in this.queues) {
		this.accounts[p] = new API3.Resources();
		this.queueArrays.push([p,this.queues[p]]);
	}
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });

	this.curItemQueue = [];
};

m.QueueManager.prototype.getAvailableResources = function(gameState, noAccounts) {
	var resources = gameState.getResources();
	if (noAccounts)
		return resources;
	for (var key in this.queues) {
		resources.subtract(this.accounts[key]);
	}
	return resources;
};

m.QueueManager.prototype.getTotalAccountedResources = function(gameState) {
	var resources = new API3.Resources();
	for (var key in this.queues) {
		resources.add(this.accounts[key]);
	}
	return resources;
};

m.QueueManager.prototype.currentNeeds = function(gameState) {
	var needed = new API3.Resources();
	//queueArrays because it's faster.
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.length() == 0 || !queue.queue[0].isGo(gameState))
			continue;
		// we need resource if the account is smaller than the cost
		var costs = queue.queue[0].getCost();
		for each (var ress in costs.types)
		    costs[ress] = Math.max(0, costs[ress] - this.accounts[name][ress]);

		needed.add(costs);
	}
	return needed;
};

m.QueueManager.prototype.futureNeeds = function(gameState) {
	var needs = new API3.Resources();
	// get out current resources, not removing accounts.
	var current = this.getAvailableResources(gameState, true);
	//queueArrays because it's faster.
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		for (var j = 0; j < queue.length(); ++j)
		{
			var costs = queue.queue[j].getCost();
			if (!queue.queue[j].isGo(gameState))
				costs.multiply(0.5);
			needs.add(costs);
		}
	}
	return {
		"food" : Math.max(25 + needs.food - current.food, 10),
		"wood" : Math.max(needs.wood - current.wood, 10),
		"stone" : Math.max(needs.stone - current.stone, 0),
		"metal" : Math.max(needs.metal - current.metal, 0)
	};
};

// calculate the gather rates we'd want to be able to start all elements in our queues
// TODO: many things.
m.QueueManager.prototype.wantedGatherRates = function(gameState, shortTerm) {
	// global rates
	var rates = { "food" : 0, "wood" : 0, "stone" : 0, "metal" : 0 };
	// per-queue.
	var qTime = gameState.getTimeElapsed();
	var time = gameState.getTimeElapsed();
	var qCosts = { "food" : 0, "wood" : 0, "stone" : 0, "metal" : 0 };

	var currentRess = this.getAvailableResources(gameState);
	
	//queueArrays because it's faster.
	for (var i in this.queueArrays)
	{
		qCosts = { "food" : 0, "wood" : 0, "stone" : 0, "metal" : 0 };
		qTime = gameState.getTimeElapsed();
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];

		// we'll move temporally along the queue.
		for (var j = 0; j < queue.length(); ++j)
		{
			var elem = queue.queue[j];
			var cost = elem.getCost();
			
			var timeMultiplier = Math.max(1,(qTime-time)/25000);
			if (shortTerm)
				timeMultiplier += 0.8;

			if (!elem.isGo(gameState))
			{
				// assume we'll be wanted in four minutes.
				// TODO: work on this.
				for (var type in qCosts)
					qCosts[type] += cost[type] / timeMultiplier;
				qTime += 240000;
				break;	// disregard other stuffs.
			}
			// Assume we want it in 30 seconds from current time.
			// Costs are made higher based on priority and lower based on current time.
			// TODO: work on this.
			for (var type in qCosts)
			{
				if (cost[type] === 0)
					continue;
				qCosts[type] += (cost[type] + Math.min(cost[type],this.priorities[name])) / timeMultiplier;
			}
			qTime += 30000;	// TODO: this needs a lot more work.
		}
		for (var j in qCosts)
		{
			qCosts[j] -= this.accounts[name][j];
			var diff = Math.min(qCosts[j], currentRess[j]);
			qCosts[j] -= diff;
			currentRess[j] -= diff;
			rates[j] += qCosts[j]/(qTime/1000);
		}
	}

	return rates;
};

/*m.QueueManager.prototype.logNeeds = function(gameState) {
 if (!this.totor)
 {
 this.totor = [];
 this.currentGathR = [];
 this.currentGathRWanted = [];
 this.ressLev = [];
}
 
	if (gameState.ai.playedTurn % 10 !== 0)
		return;
	
	
	var array = this.wantedGatherRates(gameState);
	this.totor.push( array );
	
	
	var currentRates = {};
	for (var type in array)
		currentRates[type] = 0;
	for (var i in gameState.ai.HQ.baseManagers)
	{
		var base = gameState.ai.HQ.baseManagers[i];
		for (var type in array)
		{
			base.gatherersByType(gameState,type).forEach (function (ent) { //}){
														  var worker = ent.getMetadata(PlayerID, "worker-object");
														  if (worker)
														  currentRates[type] += worker.getGatherRate(gameState);
														  });
		}
	}
	this.currentGathR.push( currentRates );
	
	var types = Object.keys(array);
	
	types.sort(function(a, b) {
			   var va = (Math.max(0,array[a] - currentRates[a]))/ (currentRates[a]+1);
			   var vb = (Math.max(0,array[b] - currentRates[b]))/ (currentRates[b]+1);
			   if (va === vb)
			   return (array[b]/(currentRates[b]+1)) - (array[a]/(currentRates[a]+1));
			   return vb-va;
			   });
	this.currentGathRWanted.push( types );

	var rss = gameState.getResources();
	this.ressLev.push( {"food" : rss["food"],"stone" : rss["stone"],"wood" : rss["wood"],"metal" : rss["metal"]} );
	
	if (gameState.getTimeElapsed() > 20*60*1000 && !this.once)
	{
		this.once = true;
		for (var j in array)
		{
			log (j + ";");
			for (var i = 0; i < this.totor.length; ++i)
			{
				log (this.totor[i][j] + ";");
			}
		}
		log();
		for (var j in array)
		{
			log (j + ";");
			for (var i = 0; i < this.totor.length; ++i)
			{
				log (this.currentGathR[i][j] + ";");
			}
		}
		log();
		for (var j in array)
		{
			log (j + ";");
			for (var i = 0; i < this.totor.length; ++i)
			{
				log (this.currentGathRWanted[i].indexOf(j) + ";");
			}
		}
		log();
		for (var j in array)
		{
			log (j + ";");
			for (var i = 0; i < this.totor.length; ++i)
			{
				log (this.ressLev[i][j] + ";");
			}
		}
	}
};
*/

m.QueueManager.prototype.printQueues = function(gameState){
	m.debug("QUEUES");
	for (var i in this.queues){
		var qStr = "";
		var q = this.queues[i];
		if (q.queue.length > 0)
			m.debug((i + ":"));
		for (var j in q.queue){
			qStr = "     " + q.queue[j].type + " ";
			if (q.queue[j].number)
				qStr += "x" + q.queue[j].number;
			m.debug (qStr);
		}
	}
	m.debug ("Accounts");
	for (var p in this.accounts)
	{
		m.debug(p + ": " + uneval(this.accounts[p]));
	}
	m.debug("Needed Resources:" + uneval(this.futureNeeds(gameState,false)));
	m.debug ("Wanted Gather Rates:" + uneval(this.wantedGatherRates(gameState)));
	m.debug ("Current Resources:" + uneval(gameState.getResources()));
	m.debug ("Available Resources:" + uneval(this.getAvailableResources(gameState)));
};

// nice readable HTML version.
m.QueueManager.prototype.HTMLprintQueues = function(gameState){
	if (!m.DebugEnabled())
		return;
	var strToSend = [];
	strToSend.push("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\"> <html> <head> <title>Aegis Queue Manager</title> <link rel=\"stylesheet\" href=\"table.css\">  </head> <body> <table> <caption>Aegis Build Order</caption> ");
	for (var i in this.queues){
		strToSend.push("<tr>");
		
		var q = this.queues[i];
		var str = "<th>" + i + "  (" + this.priorities[i] + ")<br><span class=\"ressLevel\">";
		for each (var k in this.accounts[i].types)
			if(k != "population")
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
			for each (var k in costs.types) {
				qStr += costs[k] + k.substr(0,1).toUpperCase() ;
				if (k != "metal") qStr += " / ";
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
	for each (var logged in strToSend)
		log(logged);
};

m.QueueManager.prototype.clear = function(){
	this.curItemQueue = [];
	for (var i in this.queues)
		this.queues[i].empty();
};

m.QueueManager.prototype.update = function(gameState) {
	var self = this;
	
	for (var i in this.priorities){
		if (!(this.priorities[i] > 0)){
			this.priorities[i] = 1;  // TODO: make the Queue Manager not die when priorities are zero.
			warn("QueueManager received bad priorities, please report this error: " + uneval(this.priorities));
		}
	}
	
	Engine.ProfileStart("Queue Manager");
			
	// Let's assign resources to plans that need'em
	var availableRes = this.getAvailableResources(gameState);
	for (var ress in availableRes)
	{
		if (ress === "population")
			continue;

		if (availableRes[ress] > 0)
		{
			var totalPriority = 0;
			var tempPrio = {};
			var maxNeed = {};
			// Okay so this is where it gets complicated.
			// If a queue requires "ress" for the next elements (in the queue)
			// And the account is not high enough for it.
			// Then we add it to the total priority.
			// To try and be clever, we don't want a long queue to hog all resources. So two things:
			//	-if a queue has enough of resource X for the 1st element, its priority is decreased (/2).
			//	-queues accounts are capped at "resources for the first + 80% of the next"
			// This avoids getting a high priority queue with many elements hogging all of one resource
			// uselessly while it awaits for other resources.
			for (var j in this.queues) {
				// returns exactly the correct amount, ie 0 if we're not go.
				var queueCost = this.queues[j].maxAccountWanted(gameState);
				if (this.queues[j].length() > 0 && this.accounts[j][ress] < queueCost[ress] && !this.queues[j].paused)
				{
					// check that we're not too forward in this resource compared to others.
					/*var maxp = this.accounts[j][ress] / (queueCost[ress]+1);
					var tooFull = false;
					for (var tempRess in availableRes)
						if (tempRess !== ress && queueCost[tempRess] > 0 && (this.accounts[j][tempRess] / (queueCost[tempRess]+1)) - maxp < -0.2)
							tooFull = true;
					if (tooFull)
						continue;*/
					
					// adding us to the list of queues that need an update.
					tempPrio[j] = this.priorities[j];
					maxNeed[j] = queueCost[ress] - this.accounts[j][ress];
					// if we have enough of that resource for our first item in the queue, diminish our priority.
					if (this.accounts[j][ress] >= this.queues[j].getNext().getCost()[ress])
						tempPrio[j] /= 2;

					if (tempPrio[j])
						totalPriority += tempPrio[j];
				}
				else if (this.accounts[j][ress] > queueCost[ress])
				{
					this.accounts[j][ress] = queueCost[ress];
				}
			}
			// Now we allow resources to the accounts. We can at most allow "TempPriority/totalpriority*available"
			// But we'll sometimes allow less if that would overflow.
			for (var j in tempPrio) {
				// we'll add at much what can be allowed to this queue.
				var toAdd = tempPrio[j]/totalPriority * availableRes[ress];
				var maxAdd = Math.floor(Math.min(maxNeed[j], toAdd));
				this.accounts[j][ress] += maxAdd;
			}
		} else {
			// We have no available resources, see if we can't "compact" them in one queue.
			// compare queues 2 by 2, and if one with a higher priority could be completed by our amount, give it.
			// TODO: this isn't perfect compression.
			for (var j in this.queues)
			{
				var queue = this.queues[j];
				var queueCost = queue.maxAccountWanted(gameState);
				if (this.queues[j].length() === 0 || this.queues[j].paused)
					continue;

				for (var i in this.queues)
				{
					if (i === j)
						continue;
					var otherQueue = this.queues[i];
					if (this.priorities[i] >= this.priorities[j] || otherQueue.switched !== 0)
						continue;

					for (var ress in queueCost)
					{
						if (this.accounts[j][ress] >= queueCost[ress])
							continue;
						if (this.accounts[j][ress] + this.accounts[i][ress] >= queueCost[ress])
						{
							// we would be helped by it. Check if it's worth it.
							for (var otherRess in queueCost)
								if (otherRess !== ress && queueCost[otherRess] + 100 >= queueCost[ress])
									continue;
							var diff = Math.min(queueCost[ress] - this.accounts[j][ress],this.accounts[i][ress]);
							this.accounts[j][ress] += diff;
							this.accounts[i][ress] -= diff;
							++otherQueue.switched;
							//warn ("switching " + ress + " from " + i + " to " + j + " in amount " + diff);
						}
					}
				}
			}
		}
	}
	
	Engine.ProfileStart("Pick items from queues");

	//m.debug ("start");
	//m.debug (uneval(this.accounts));
	// Start the next item in the queue if we can afford it.
	for (var i in this.queueArrays)
	{
		var name = this.queueArrays[i][0];
		var queue = this.queueArrays[i][1];
		if (queue.length() > 0 && !queue.paused)
		{
			var item = queue.getNext();
			var total = new API3.Resources();
			total.add(this.accounts[name]);
			if (total.canAfford(item.getCost()))
			{
				if (item.canStart(gameState))
				{
					this.accounts[name].subtract(item.getCost());
					queue.startNext(gameState);
					queue.switched = 0;
				}
			}
		} else if (queue.length() === 0) {
			this.accounts[name].reset();
			queue.switched = 0;
		}
	}
	//m.debug (uneval(this.accounts));
	
	Engine.ProfileStop();
	
	if (gameState.ai.playedTurn % 15 === 2)
		this.HTMLprintQueues(gameState);

	Engine.ProfileStop();
};

m.QueueManager.prototype.pauseQueue = function(queue, scrapAccounts) {
	if (this.queues[queue])
	{
		this.queues[queue].paused = true;
		if (scrapAccounts)
			this.accounts[queue].reset();
	}
}

m.QueueManager.prototype.unpauseQueue = function(queue) {
	if (this.queues[queue])
		this.queues[queue].paused = false;
}

m.QueueManager.prototype.pauseAll = function(scrapAccounts, but) {
	for (var p in this.queues)
		if (p != but)
		{
			if (scrapAccounts)
				this.accounts[p].reset();
			this.queues[p].paused = true;
		}
}

m.QueueManager.prototype.unpauseAll = function(but) {
	for (var p in this.queues)
		if (p != but)
			this.queues[p].paused = false;
}


m.QueueManager.prototype.addQueue = function(queueName, priority) {
	if (this.queues[queueName] == undefined) {
		this.queues[queueName] = new m.Queue();
		this.priorities[queueName] = priority;
		this.accounts[queueName] = new API3.Resources();

		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
}
m.QueueManager.prototype.removeQueue = function(queueName) {
	if (this.queues[queueName] !== undefined) {
		if ( this.curItemQueue.indexOf(queueName) !== -1) {
			this.curItemQueue.splice(this.curItemQueue.indexOf(queueName),1);
		}
		delete this.queues[queueName];
		delete this.priorities[queueName];
		delete this.accounts[queueName];
		
		var self = this;
		this.queueArrays = [];
		for (var p in this.queues)
			this.queueArrays.push([p,this.queues[p]]);
		this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
	}
}
m.QueueManager.prototype.changePriority = function(queueName, newPriority) {
	var self = this;
	if (this.queues[queueName] !== undefined)
		this.priorities[queueName] = newPriority;
	this.queueArrays = [];
	for (var p in this.queues)
		this.queueArrays.push([p,this.queues[p]]);
	this.queueArrays.sort(function (a,b) { return (self.priorities[b[0]] - self.priorities[a[0]]) });
}

return m;
}(AEGIS);
