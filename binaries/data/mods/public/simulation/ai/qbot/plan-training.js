var UnitTrainingPlan = function(gameState, type, metadata, number) {
	this.type = gameState.applyCiv(type);
	this.metadata = metadata;

	var template = gameState.getTemplate(this.type);
	if (!template) {
		this.invalidTemplate = true;
		return;
	}
	this.category= "unit";
	this.cost = new Resources(template.cost(), template._template.Cost.Population);
	if (!number){
		this.number = 1;
	}else{
		this.number = number;
	}
};

UnitTrainingPlan.prototype.canExecute = function(gameState) {
	if (this.invalidTemplate)
		return false;

	// TODO: we should probably check pop caps

	var trainers = gameState.findTrainers(this.type);

	return (trainers.length != 0);
};

UnitTrainingPlan.prototype.execute = function(gameState) {
	//warn("Executing UnitTrainingPlan " + uneval(this));

	var trainers = gameState.findTrainers(this.type).toEntityArray();

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0){
		trainers.sort(function(a, b) {
			return a.trainingQueueTime() - b.trainingQueueTime();
		});
	
		trainers[0].train(this.type, this.number, this.metadata);
	}
};

UnitTrainingPlan.prototype.getCost = function(){
	var multCost = new Resources();
	multCost.add(this.cost);
	multCost.multiply(this.number);
	return multCost;
};

UnitTrainingPlan.prototype.addItem = function(){
	this.number += 1;
};