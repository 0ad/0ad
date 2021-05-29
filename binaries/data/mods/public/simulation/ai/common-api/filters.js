var API3 = function(m)
{

m.Filters = {
	"byType": type => ({
		"func": ent => ent.templateName() == type,
		"dynamicProperties": []
	}),

	"byClass": cls => ({
		"func": ent => ent.hasClass(cls),
		"dynamicProperties": []
	}),

	"byClasses": clsList => ({
		"func": ent => ent.hasClasses(clsList),
		"dynamicProperties": []
	}),

	"byMetadata": (player, key, value) => ({
		"func": ent => ent.getMetadata(player, key) == value,
		"dynamicProperties": ['metadata.' + key]
	}),

	"byHasMetadata": (player, key) => ({
		"func": ent => ent.getMetadata(player, key) !== undefined,
		"dynamicProperties": ['metadata.' + key]
	}),

	"and": (filter1, filter2) => ({
		"func": ent => filter1.func(ent) && filter2.func(ent),
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)
	}),

	"or": (filter1, filter2) => ({
		"func": ent => filter1.func(ent) || filter2.func(ent),
		"dynamicProperties": filter1.dynamicProperties.concat(filter2.dynamicProperties)
	}),

	"not": (filter) => ({
		"func": ent => !filter.func(ent),
		"dynamicProperties": filter.dynamicProperties
	}),

	"byOwner": owner => ({
		"func": ent => ent.owner() == owner,
		"dynamicProperties": ['owner']
	}),

	"byNotOwner": owner => ({
		"func": ent => ent.owner() != owner,
		"dynamicProperties": ['owner']
	}),

	"byOwners": owners => ({
		"func": ent => owners.some(owner => owner == ent.owner()),
		"dynamicProperties": ['owner']
	}),

	"byCanGarrison": () => ({
		"func": ent => ent.garrisonMax() > 0,
		"dynamicProperties": []
	}),

	"byTrainingQueue": () => ({
		"func": ent => ent.trainingQueue(),
		"dynamicProperties": ['trainingQueue']
	}),

	"byResearchAvailable": (gameState, civ) => ({
		"func": ent => ent.researchableTechs(gameState, civ) !== undefined,
		"dynamicProperties": []
	}),

	"byCanAttackClass": aClass => ({
		"func": ent => ent.canAttackClass(aClass),
		"dynamicProperties": []
	}),

	"byCanAttackTarget": target => ({
		"func": ent => ent.canAttackTarget(target),
		"dynamicProperties": []
	}),

	"isGarrisoned": () => ({
		"func": ent => ent.position() === undefined,
		"dynamicProperties": []
	}),

	"isIdle": () => ({
		"func": ent => ent.isIdle(),
		"dynamicProperties": ['idle']
	}),

	"isFoundation": () => ({
		"func": ent => ent.foundationProgress() !== undefined,
		"dynamicProperties": []
	}),

	"isBuilt": () => ({
		"func": ent => ent.foundationProgress() === undefined,
		"dynamicProperties": []
	}),

	"hasDefensiveFire": () => ({
		"func": ent => ent.hasDefensiveFire(),
		"dynamicProperties": []
	}),

	"isDropsite": resourceType => ({
		"func": ent => ent.isResourceDropsite(resourceType),
		"dynamicProperties": []
	}),

	"isTreasure": () => ({
		"func": ent => {
			if (!ent.isTreasure())
				return false;

			// Don't go for floating treasures since we might not be able
			// to reach them and that kills the pathfinder.
			let template = ent.templateName();
			return template != "gaia/treasure/shipwreck_debris" &&
			    template != "gaia/treasure/shipwreck";
		},
		"dynamicProperties": []
	}),

	"byResource": resourceType => ({
		"func": ent => {
			if (!ent.resourceSupplyMax())
				return false;

			let type = ent.resourceSupplyType();
			if (!type)
				return false;

			// Skip targets that are too hard to hunt
			if (!ent.isHuntable() || ent.hasClass("SeaCreature"))
				return false;

			return resourceType == type.generic;
		},
		"dynamicProperties": []
	}),

	"isHuntable": () => ({
		// Skip targets that are too hard to hunt and don't go for the fish! TODO: better accessibility checks
		"func": ent => ent.hasClass("Animal") && ent.resourceSupplyMax() &&
		               ent.isHuntable() && !ent.hasClass("SeaCreature"),
		"dynamicProperties": []
	}),

	"isFishable": () => ({
		// temporarily do not fish moving fish (i.e. whales)
		"func": ent => !ent.get("UnitMotion") && ent.hasClass("SeaCreature") && ent.resourceSupplyMax(),
		"dynamicProperties": []
	})
};

return m;

}(API3);

