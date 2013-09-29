/*
 * Holds a list of wanted items to train or construct
 */

var Queue = function() {
	this.queue = [];
	this.paused = false;
};

Queue.prototype.empty = function() {
	this.queue = [];
};

Queue.prototype.addItem = function(plan) {
	for (var i in this.queue)
	{
		if (plan.category === "unit" && this.queue[i].type == plan.type && this.queue[i].number + plan.number <= this.queue[i].maxMerge)
		{
			this.queue[i].addItem(plan.number)
			return;
		}
	}
	this.queue.push(plan);
};

Queue.prototype.getNext = function() {
	if (this.queue.length > 0) {
		return this.queue[0];
	} else {
		return null;
	}
};

Queue.prototype.startNext = function(gameState) {
	if (this.queue.length > 0) {
		this.queue.shift().start(gameState);
		return true;
	} else {
		return false;
	}
};

// returns the maximal account we'll accept for this queue.
// Currently 100% of the cost of the first element and 80% of that of the second
Queue.prototype.maxAccountWanted = function(gameState) {
	var cost = new Resources();
	if (this.queue.length > 0 && this.queue[0].isGo(gameState))
		cost.add(this.queue[0].getCost());
	if (this.queue.length > 1 && this.queue[1].isGo(gameState))
	{
		var costs = this.queue[1].getCost();
		costs.multiply(0.8);
		cost.add(costs);
	}
	return cost;
};

Queue.prototype.queueCost = function(){
	var cost = new Resources();
	for (var key in this.queue){
		cost.add(this.queue[key].getCost());
	}
	return cost;
};

Queue.prototype.length = function() {
	return this.queue.length;
};

Queue.prototype.countQueuedUnits = function(){
	var count = 0;
	for (var i in this.queue){
		count += this.queue[i].number;
	}
	return count;
};

Queue.prototype.countQueuedUnitsWithClass = function(classe){
	var count = 0;
	for (var i in this.queue){
		if (this.queue[i].template && this.queue[i].template.hasClass(classe))
			count += this.queue[i].number;
	}
	return count;
};
Queue.prototype.countQueuedUnitsWithMetadata = function(data,value){
	var count = 0;
	for (var i in this.queue){
		if (this.queue[i].metadata[data] && this.queue[i].metadata[data] == value)
			count += this.queue[i].number;
	}
	return count;
};

Queue.prototype.countAllByType = function(t){
	var count = 0;
	
	for (var i = 0; i < this.queue.length; i++){
		if (this.queue[i].type === t){
			count += this.queue[i].number;
		} 
	}
	return count;
};
