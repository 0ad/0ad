var API3 = function(m)
{

m.Filters = {
	byType: function(type){
		return {"func" : function(ent){
			return ent.templateName() === type;
		},
		"dynamicProperties": []};
	},
	
	byClass: function(cls){
		return {"func" : function(ent){
			return ent.hasClass(cls);
		},
		"dynamicProperties": []};
	},
	
	byClassesAnd: function(clsList){
		return {"func" : function(ent){
			var ret = true;
			for (var i in clsList){
				ret = ret && ent.hasClass(clsList[i]);
			}
			return ret;
		}, 
		"dynamicProperties": []};
	},
	
	byClassesOr: function(clsList){
		return {"func" : function(ent){
			var ret = false;
			for (var i in clsList){
				ret = ret || ent.hasClass(clsList[i]);
			}
			return ret;
		}, 
		"dynamicProperties": []};
	},

	byMetadata: function(player, key, value){
		return {"func" : function(ent){
			return (ent.getMetadata(player, key) == value);
		}, 
		"dynamicProperties": ['metadata.' + key]};
	},

	// can be used for stuffs which won't change once entities are created.
	byStaticMetadata: function(player, key, value){
		return {"func" : function(ent){
			return (ent.getMetadata(player, key) == value);
		},
			"dynamicProperties": []};
	},
	
	byHasMetadata: function(player, key){
		return {"func" : function(ent){
			return (ent.getMetadata(player, key) != undefined);
		},
			"dynamicProperties": ['metadata.' + key]};
	},

	and: function(filter1, filter2){
		return {"func": function(ent){
			return filter1.func(ent) && filter2.func(ent);
		}, 
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)};
	},
	
	or: function(filter1, filter2){
		return {"func" : function(ent){
			return filter1.func(ent) || filter2.func(ent);
		}, 
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)};
	},
	
	not: function(filter){
		return {"func": function(ent){
			return !filter.func(ent);
		},
		"dynamicProperties": filter.dynamicProperties};
	},
	
	byOwner: function(owner){
		return {"func" : function(ent){
			return (ent.owner() === owner);
		}, 
		"dynamicProperties": ['owner']};
	},
	
	byNotOwner: function(owner){
		return {"func" : function(ent){
			return (ent.owner() !== owner);
		}, 
		"dynamicProperties": ['owner']};
	},
	
	byOwners: function(owners){
		return {"func" : function(ent){
			for (var i in owners){
				if (ent.owner() == +owners[i]){
					return true;
				}
			}
			return false;
		}, 
		"dynamicProperties": ['owner']};
	},

	byCanGarrison: function(){
		return {"func" : function(ent){
			return ent.garrisonMax() > 0;
		},
			"dynamicProperties": []};
	},
	byTrainingQueue: function(){
		return {"func" : function(ent){
			return ent.trainingQueue();
		}, 
		"dynamicProperties": ['trainingQueue']};
	},
	byResearchAvailable: function(){
		return {"func" : function(ent){
			return ent.researchableTechs() !== undefined;
		},
			"dynamicProperties": []};
	},
	byTargetedEntity: function(targetID){
		return {"func": function(ent) {
			return (ent.unitAIOrderData().length && ent.unitAIOrderData()[0]["target"] && ent.unitAIOrderData()[0]["target"] == targetID);
		},
			"dynamicProperties": ['unitAIOrderData']};
	},
	
	byCanAttack: function(saidClass){
		return {"func" : function(ent){
			return ent.canAttackClass(saidClass);
		},
			"dynamicProperties": []};
	},
	
	isGarrisoned: function(){
		return {"func" : function(ent){
			return ent.position() === undefined;
		},
			"dynamicProperties": []};
	},

	isSoldier: function(){
		return {"func" : function(ent){
			return Filters.byClassesOr(["CitizenSoldier", "Super"])(ent);
		}, 
		"dynamicProperties": []};
	},
	
	isIdle: function(){
		return {"func" : function(ent){
			return ent.isIdle();
		}, 
		"dynamicProperties": ['idle']};
	},
	
	isFoundation: function(){
		return {"func": function(ent){
			return ent.foundationProgress() !== undefined;
		},
		"dynamicProperties": []};
	},
	
	byDistance: function(startPoint, dist){
		return {"func": function(ent){
			if (ent.position() === undefined){
				return false;
			}else{
				return (m.SquareVectorDistance(startPoint, ent.position()) < dist*dist);
			}
		},
		"dynamicProperties": ['position']};
	},
	
	// Distance filter with no auto updating, use with care 
	byStaticDistance: function(startPoint, dist){
		return {"func": function(ent){
			if (!ent.position()){
				return false;
			}else{
				return (m.SquareVectorDistance(startPoint, ent.position()) < dist*dist);
			}
		},
		"dynamicProperties": []};
	},
	
	byTerritory: function(Map, territoryIndex){
		return {"func": function(ent){
			if (Map.point(ent.position()) == territoryIndex) {
				return true;
			} else {
				return false;
			}
		},
		"dynamicProperties": ['position']};
	},

	isDropsite: function(resourceType){
		return {"func": function(ent){
			return (ent.resourceDropsiteTypes() && (resourceType === undefined || ent.resourceDropsiteTypes().indexOf(resourceType) !== -1));
		},
		"dynamicProperties": []};
	},
	
	byResource: function(resourceType){
		return {"func" : function(ent){
			var type = ent.resourceSupplyType();
			if (!type)
				return false;
			var amount = ent.resourceSupplyMax();
			if (!amount)
				return false;
			
			// Skip targets that are too hard to hunt
			if (ent.isUnhuntable())
				return false;
			
			// And don't go for the bloody fish! TODO: better accessibility checks 
			if (ent.hasClass("SeaCreature"))
				return false;
			
			// Don't go for floating treasures since we won't be able to reach them and it kills the pathfinder.
			if (ent.templateName() == "other/special_treasure_shipwreck_debris" || 
					ent.templateName() == "other/special_treasure_shipwreck" )
				return false;
			
			if (type.generic == "treasure")
				return (resourceType == type.specific);
			else
				return (resourceType == type.generic);
		},
		"dynamicProperties": []};
	},

	isHuntable: function(){
		return {"func" : function(ent){
			if (!ent.hasClass("Animal"))
				return false;
			var amount = ent.resourceSupplyMax();
			if (!amount)
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

	isFishable: function(){
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

