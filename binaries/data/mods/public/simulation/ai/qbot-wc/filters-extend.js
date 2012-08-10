// Some new filters I use in entity Collections

Filters["byID"] = 
	function(id){
		return {"func": function(ent){
			return (ent.id() == id);
	}, 
	"dynamicProperties": ['id']};
};
Filters["byTargetedEntity"] = 
function(targetID){
	return {"func": function(ent){
		return (ent.unitAIOrderData() && ent.unitAIOrderData()["target"] && ent.unitAIOrderData()["target"] == targetID);
	}, 
	"dynamicProperties": ['unitAIOrderData']};
};
Filters["byHasMetadata"] =
function(key){
	return {"func" : function(ent){
		return (ent.getMetadata(key) != undefined);
	}, 
		"dynamicProperties": ['metadata.' + key]};
};
Filters["byTerritory"] = function(Map, territoryIndex){
	return {"func": function(ent){
		if (Map.point(ent.position()) == territoryIndex) {
			return true;
		} else {
			return false;
		}
	},
		"dynamicProperties": ['position']};
};
Filters["isDropsite"] = function(resourceType){
	return {"func": function(ent){
		return (ent.resourceDropsiteTypes() && ent.resourceDropsiteTypes().indexOf(resourceType) !== -1
				&& ent.foundationProgress() === undefined);
	},
		"dynamicProperties": []};
};


