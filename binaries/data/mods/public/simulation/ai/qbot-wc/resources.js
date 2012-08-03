function Resources(amounts, population) {
	if (amounts === undefined) {
		amounts = {
			food : 0,
			wood : 0,
			stone : 0,
			metal : 0
		};
	}
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		this[t] = amounts[t] || 0;
	}

	if (population > 0) {
		this.population = parseInt(population);
	} else {
		this.population = 0;
	}
}

Resources.prototype.types = [ "food", "wood", "stone", "metal" ];

Resources.prototype.canAfford = function(that) {
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		if (this[t] < that[t]) {
			return false;
		}
	}
	return true;
};

Resources.prototype.add = function(that) {
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		this[t] += that[t];
	}
	this.population += that.population;
};

Resources.prototype.subtract = function(that) {
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		this[t] -= that[t];
	}
	this.population += that.population;
};

Resources.prototype.multiply = function(n) {
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		this[t] *= n;
	}
	this.population *= n;
};

Resources.prototype.toInt = function() {
	var sum = 0;
	for ( var tKey in this.types) {
		var t = this.types[tKey];
		sum += this[t];
	}
	sum += this.population * 50; // based on typical unit costs
	return sum;
};
