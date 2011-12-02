/*
 * Holds a list of wanted items to train or construct
 */

var Queue = function() {
	this.queue = [];
	this.outQueue = [];
};

Queue.prototype.addItem = function(plan) {
	this.queue.push(plan);
};

Queue.prototype.getNext = function() {
	if (this.queue.length > 0) {
		return this.queue[0];
	} else {
		return null;
	}
};

Queue.prototype.outQueueNext = function(){
	if (this.outQueue.length > 0) {
		return this.outQueue[0];
	} else {
		return null;
	}
};

Queue.prototype.outQueueCost = function(){
	var cost = new Resources();
	for (key in this.outQueue){
		cost.add(this.outQueue[key].getCost());
	}
	return cost;
};

Queue.prototype.nextToOutQueue = function(){
	if (this.queue.length > 0){
		if (this.outQueue.length > 0 && 
				this.getNext().category === "unit" && 
				this.outQueue[this.outQueue.length-1].type === this.getNext().type &&
				this.outQueue[this.outQueue.length-1].number < 5){
			this.queue.shift();
			this.outQueue[this.outQueue.length-1].addItem();
		}else{
			this.outQueue.push(this.queue.shift());
		}
	}
};

Queue.prototype.executeNext = function(gameState) {
	if (this.outQueue.length > 0) {
		this.outQueue.shift().execute(gameState);
		return true;
	} else {
		return false;
	}
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

Queue.prototype.countOutQueuedUnits = function(){
	var count = 0;
	for (var i in this.outQueue){
		count += this.outQueue[i].number;
	}
	return count;
};

Queue.prototype.countTotalQueuedUnits = function(){
	var count = 0;
	for (var i in this.queue){
		count += this.queue[i].number;
	}
	for (var i in this.outQueue){
		count += this.outQueue[i].number;
	}
	return count;
};

Queue.prototype.totalLength = function(){
	return this.queue.length + this.outQueue.length;
};

Queue.prototype.outQueueLength = function(){
	return this.outQueue.length;
};

Queue.prototype.countAllByType = function(t){
	var count = 0;
	
	for (var i = 0; i < this.queue.length; i++){
		if (this.queue[i].type === t){
			count += this.queue[i].number;
		} 
	}
	for (var i = 0; i < this.outQueue.length; i++){
		if (this.outQueue[i].type === t){
			count += this.outQueue[i].number;
		} 
	}
	return count;
};