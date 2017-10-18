var API3 = function(m)
{

m.Filters = {
	"byType": function(type){
		return {"func" : function(ent){
			return ent.templateName() === type;
		},
		"dynamicProperties": []};
	},

	"byClass": function(cls){
		return {"func" : function(ent){
			return ent.hasClass(cls);
		},
		"dynamicProperties": []};
	},

	"byClassesAnd": function(clsList){
		return {"func" : function(ent){
			let ret = true;
			for (let cls of clsList)
				ret = ret && ent.hasClass(cls);
			return ret;
		},
		"dynamicProperties": []};
	},

	"byClassesOr": function(clsList){
		return {"func" : function(ent){
			let ret = false;
			for (let cls of clsList)
				ret = ret || ent.hasClass(cls);
			return ret;
		},
		"dynamicProperties": []};
	},

	"byMetadata": function(player, key, value){
		return {"func" : function(ent){
			return ent.getMetadata(player, key) == value;
		},
		"dynamicProperties": ['metadata.' + key]};
	},

	"byHasMetadata": function(player, key){
		return {"func" : function(ent){
			return ent.getMetadata(player, key) !== undefined;
		},
		"dynamicProperties": ['metadata.' + key]};
	},

	"and": function(filter1, filter2){
		return {"func": function(ent){
			return filter1.func(ent) && filter2.func(ent);
		},
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)};
	},

	"or": function(filter1, filter2){
		return {"func" : function(ent){
			return filter1.func(ent) || filter2.func(ent);
		},
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)};
	},

	"not": function(filter){
		return {"func": function(ent){
			return !filter.func(ent);
		},
		"dynamicProperties": filter.dynamicProperties};
	},

	"byOwner": function(owner){
		return {"func" : function(ent){
			return ent.owner() === owner;
		},
		"dynamicProperties": ['owner']};
	},

	"byNotOwner": function(owner){
		return {"func" : function(ent){
			return ent.owner() !== owner;
		},
		"dynamicProperties": ['owner']};
	},

	"byOwners": function(owners){
		return {"func" : function(ent){
			for (let owner of owners)
				if (ent.owner() === owner)
					return true;
			return false;
		},
		"dynamicProperties": ['owner']};
	},

	"byCanGarrison": function(){
		return {"func" : function(ent){
			return ent.garrisonMax() > 0;
		},
		"dynamicProperties": []};
	},

	"byTrainingQueue": function(){
		return {"func" : function(ent){
			return ent.trainingQueue();
		},
		"dynamicProperties": ['trainingQueue']};
	},

	"byResearchAvailable": function(civ){
		return {"func" : function(ent){
			return ent.researchableTechs(civ) !== undefined;
		},
		"dynamicProperties": []};
	},

	"byCanAttackClass": function(aClass)
	{
		return {"func" : function(ent){
			return ent.canAttackClass(aClass);
		},
		"dynamicProperties": []};
	},

	"isGarrisoned": function(){
		return {"func" : function(ent){
			return ent.position() === undefined;
		},
		"dynamicProperties": []};
	},

	"isIdle": function(){
		return {"func" : function(ent){
			return ent.isIdle();
		},
		"dynamicProperties": ['idle']};
	},

	"isFoundation": function(){
		return {"func": function(ent){
			return ent.foundationProgress() !== undefined;
		},
		"dynamicProperties": []};
	},

	"isBuilt": function(){
		return {"func": function(ent){
			return ent.foundationProgress() === undefined;
		},
		"dynamicProperties": []};
	},

	"hasDefensiveFire": function(){
		return {"func": function(ent){
			return ent.hasDefensiveFire();
		},
		"dynamicProperties": []};
	},

	"isDropsite": function(resourceType){
		return {"func": function(ent){
			return ent.resourceDropsiteTypes() && (resourceType === undefined || ent.resourceDropsiteTypes().indexOf(resourceType) !== -1);
		},
		"dynamicProperties": []};
	},

	"byResource": function(resourceType){
		return {"func" : function(ent){
			if (!ent.resourceSupplyMax())
				return false;

			let type = ent.resourceSupplyType();
			if (!type)
				return false;

			// Skip targets that are too hard to hunt
			if (!ent.isHuntable() || ent.hasClass("SeaCreature"))
				return false;

			// Don't go for floating treasures since we won't be able to reach them and it kills the pathfinder.
			if (ent.templateName() == "other/special_treasure_shipwreck_debris" ||
			    ent.templateName() == "other/special_treasure_shipwreck" )
				return false;

			if (type.generic == "treasure")
				return resourceType == type.specific;

			return resourceType == type.generic;
		},
		"dynamicProperties": []};
	},

	"isHuntable": function(){
		return {"func" : function(ent){
			if (!ent.hasClass("Animal"))
				return false;
			if (!ent.resourceSupplyMax())
				return false;
			// Skip targets that are too hard to hunt
			if (!ent.isHuntable())
				return false;
			// And don't go for the fish! TODO: better accessibility checks
			if (ent.hasClass("SeaCreature"))
				return false;
			return true;
		},
		"dynamicProperties": []};
	},

	"isFishable": function(){
		return {"func" : function(ent){
			if (ent.get("UnitMotion"))   // temporarily do not fish moving fish (i.e. whales)
				return false;
			if (ent.hasClass("SeaCreature") && ent.resourceSupplyMax())
				return true;
			return false;
		},
		"dynamicProperties": []};
	}
};

return m;

}(API3);

