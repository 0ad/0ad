var Filters = {
	byClass: function(cls){
		return function(ent){
			return ent.hasClass(cls);
		};
	},
	
	byClassesAnd: function(clsList){
		return function(ent){
			var ret = true;
			for (var i in clsList){
				ret = ret && ent.hasClass(clsList[i]);
			}
			return ret;
		};
	},
	
	byClassesOr: function(clsList){
		return function(ent){
			var ret = false;
			for (var i in clsList){
				ret = ret || ent.hasClass(clsList[i]);
			}
			return ret;
		};
	},
	
	and:  function(filter1, filter2){
		return function(ent, gameState){
			return filter1(ent, gameState) && filter2(ent, gameState);
		};
	},
	
	or:  function(filter1, filter2){
		return function(ent, gameState){
			return filter1(ent, gameState) || filter2(ent, gameState);
		};
	},
	
	isEnemy: function(){
		return function(ent, gameState){
			return gameState.isEntityEnemy(ent);
		};
	},
	
	isSoldier: function(){
		return function(ent){
			return Filters.byClassesOr(["CitizenSoldier", "Super"])(ent);
		};
	}
};
