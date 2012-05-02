var Filters = {
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

	byMetadata: function(key, value){
		return {"func" : function(ent){
			return (ent.getMetadata(key) == value);
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
			return (ent.owner() != owner);
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
	
	byTrainingQueue: function(){
		return {"func" : function(ent){
			return ent.trainingQueue();
		}, 
		"dynamicProperties": ['trainingQueue']};
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
			if (!ent.position()){
				return false;
			}else{
				return (VectorDistance(startPoint, ent.position()) < dist);
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
				return (VectorDistance(startPoint, ent.position()) < dist);
			}
		},
		"dynamicProperties": []};
	},
	
	isDropsite: function(resourceType){
		return {"func": function(ent){
			return (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf(resourceType) !== -1);
		},
		"dynamicProperties": []};
	},
	
	byResource: function(resourceType){
		return {"func" : function(ent){
			var type = ent.resourceSupplyType();
			if (!type)
				return false;
			var amount = ent.resourceSupplyAmount();
			if (!amount)
				return false;
			
			// Skip targets that are too hard to hunt
			if (ent.isUnhuntable())
				return false;
			
			// And don't go for the bloody fish! TODO: better accessibility checks 
			if (ent.hasClass("SeaCreature")){
				return false;
			}
			
			// Don't go for floating treasures since we won't be able to reach them and it kills the pathfinder.
			if (ent.templateName() == "other/special_treasure_shipwreck_debris" || 
					ent.templateName() == "other/special_treasure_shipwreck" ){
				return false;
			}
			
			// Don't gather enemy farms
			if (!ent.isOwn() && ent.owner() !== 0){
				return false;
			}
			
			if (ent.getMetadata("inaccessible") === true){
				return false;
			}
			
			if (type.generic == "treasure"){
				return (resourceType == type.specific);
			} else {
				return (resourceType == type.generic);
			}
		},
		"dynamicProperties": ["resourceSupplyAmount", "owner"]};
	}
};
