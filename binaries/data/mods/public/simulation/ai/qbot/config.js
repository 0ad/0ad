var baseConfig = {
	"attack" : {
		"minAttackSize" : 20, // attackMoveToLocation
		"maxAttackSize" : 60, // attackMoveToLocation
		"enemyRatio" : 1.5, // attackMoveToLocation
		"groupSize" : 10 // military
	},
	
	// defence
	"defence" : {
		"acquireDistance" : 220,
		"releaseDistance" : 250,
		"groupRadius" : 20,
		"groupBreakRadius" : 40,
		"groupMergeRadius" : 10,
		"defenderRatio" : 2
	},
	
	// military
	"buildings" : {
		"moderate" : {
			"default" : [ "structures/{civ}_barracks" ]
		},
		"advanced" : {
			"hele" : [ "structures/{civ}_gymnasion", "structures/{civ}_fortress" ],
			"athen" : [ "structures/{civ}_gymnasion", "structures/{civ}_fortress" ],
			"spart" : [ "structures/{civ}_syssiton", "structures/{civ}_fortress" ],
			"mace" : [ "structures/{civ}_fortress" ],
			"cart" : [ "structures/{civ}_fortress", "structures/{civ}_embassy_celtic",
					"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"celt" : [ "structures/{civ}_kennel", "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ],
			"iber" : [ "structures/{civ}_fortress" ],
			"pers" : [ "structures/{civ}_fortress", "structures/{civ}_stables", "structures/{civ}_apadana" ],
			"rome" : [ "structures/{civ}_army_camp", "structures/{civ}_fortress" ]
		},
		"fort" : {
			"default" : [ "structures/{civ}_fortress" ],
			"celt" : [ "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ]
		}
	},

	// qbot
	"priorities" : {  // Note these are dynamic, you are only setting the initial values
		"house" : 500,
		"citizenSoldier" : 100,
		"villager" : 100,
		"economicBuilding" : 30,
		"field" : 20,
		"advancedSoldier" : 30,
		"siege" : 10,
		"militaryBuilding" : 50,
		"defenceBuilding" : 17,
		"civilCentre" : 1000
	},
	
	"debug" : false
};

var Config = {
		"debug": false
};

Config.__proto__ = baseConfig;