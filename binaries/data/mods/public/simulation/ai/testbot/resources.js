var Resources = Class({

	types: ["food", "wood", "stone", "metal"],

	_init: function(amounts)
	{
		for each (var t in this.types)
			this[t] = amounts[t] || 0;
	},

	canAfford: function(that)
	{
		for each (var t in this.types)
			if (this[t] < that[t])
				return false;
		return true;
	},

	add: function(that)
	{
		for each (var t in this.types)
			this[t] += that[t];
	},

	subtract: function(that)
	{
		for each (var t in this.types)
			this[t] -= that[t];
	},

	multiply: function(n)
	{
		for each (var t in this.types)
			this[t] *= n;
	},
});
