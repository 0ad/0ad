var UnitTrainingPlan = function(gameState, type, metadata, number) {
	this.type = gameState.applyCiv(type);
	this.metadata = metadata;

	this.template = gameState.getTemplate(this.type);
	if (!this.template) {
		this.invalidTemplate = true;
		this.template = undefined;
		return;
	}
	this.category= "unit";
	this.cost = new Resources(this.template.cost(), this.template._template.Cost.Population);
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
	var self = this;
	var trainers = gameState.findTrainers(this.type).toEntityArray();

	// Prefer training buildings with short queues
	// (TODO: this should also account for units added to the queue by
	// plans that have already been executed this turn)
	if (trainers.length > 0){
		trainers.sort(function(a, b) {
			
			if (self.metadata["plan"] !== undefined) {
				var aa = a.trainingQueueTime();
				var bb = b.trainingQueueTime();
				if (a.hasClass("Civic"))
					aa += 20;
				if (b.hasClass("Civic"))
					bb += 20;
				return (a.trainingQueueTime() - b.trainingQueueTime());
			}
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