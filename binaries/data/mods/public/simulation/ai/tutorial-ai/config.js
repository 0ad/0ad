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
			"cart" : [ "structures/{civ}_fortress", "structures/{civ}_embassy_celtic",
					"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"iber" : [ "structures/{civ}_fortress" ],
			"pers" : [ "structures/{civ}_fortress", "structures/{civ}_stables", "structures/{civ}_apadana" ],
			"rome" : [ "structures/{civ}_army_camp", "structures/{civ}_fortress" ]
		},
		"fort" : {
			"default" : [ "structures/{civ}_fortress" ],
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
		"debug": true
};

Config.__proto__ = baseConfig;
