// Baseconfig is the highest difficulty.
var baseConfig = {
	"Military" : {
		"fortressStartTime" : 840, // Time to wait before building one fortress.
		"fortressLapseTime" : 300, // Time to wait between building 2 fortresses (minimal)
		"defenceBuildingTime" : 300, // Time to wait before building towers or fortresses
		"advancedMilitaryStartTime" : 720, // Time to wait before building advanced military buildings. Also limited by phase 2.
		"attackPlansStartTime" : 0	// time to wait before attacking. Start as soon as possible (first barracks)
	},
	"Economy" : {
		"townPhase" : 180,	// time to start trying to reach town phase (might be a while after. Still need the requirements + ress )
		"cityPhase" : 540,	// time to start trying to reach city phase
		"farmsteadStartTime" : 240,	// Time to wait before building a farmstead.
		"marketStartTime" : 620, // Time to wait before building the market.
		"dockStartTime" : 240,	// Time to wait before building the dock
		"techStartTime" : 600,	// time to wait before teching.
		"targetNumBuilders" : 1.5, // Base number of builders per foundation. Later updated, but this remains a multiplier.
		"femaleRatio" : 0.4 // percent of females among the workforce.
	},
	
	// Note: attack settings are set directly in attack_plan.js
	
	// defence
	"Defence" : {
		"defenceRatio" : 3,	// see defence.js for more info.
		"armyCompactSize" : 700,	// squared. Half-diameter of an army.
		"armyBreakawaySize" : 900  // squared.
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
			"rome" : [ "structures/{civ}_army_camp", "structures/{civ}_fortress" ],
			"maur" : [ "structures/{civ}_elephant_stables", "structures/{civ}_fortress" ]
		},
		"fort" : {
			"default" : [ "structures/{civ}_fortress" ],
			"celt" : [ "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ]
		}
	},

	// qbot
	"priorities" : {  // Note these are dynamic, you are only setting the initial values
		"house" : 250,
		"citizenSoldier" : 50,
		"villager" : 60,
		"economicBuilding" : 80,
		"dropsites" : 180,
		"field" : 500,
		"militaryBuilding" : 120,
		"defenceBuilding" : 17,
		"majorTech" : 100,
		"minorTech" : 40,
		"civilCentre" : 10000	// will hog all resources
	},
	"difficulty" : 2,	// for now 2 is "hard", ie default. 1 is normal, 0 is easy.
	"debug" : false
};

var Config = {
	"debug": true,
	"difficulty" : 2
};

Config.__proto__ = baseConfig;

// changing settings based on difficulty.

if (Config.difficulty === 1)
{
	Config["Military"] = {
		"fortressStartTime" : 1000,
		"fortressLapseTime" : 400,
		"defenceBuildingTime" : 350,
		"advancedMilitaryStartTime" : 1000,
		"attackPlansStartTime" : 600
	};
	Config["Economy"] = {
		"townPhase" : 240,
		"cityPhase" : 660,
		"farmsteadStartTime" : 600,
		"marketStartTime" : 800,
		"techStartTime" : 1320,
		"targetNumBuilders" : 2,
		"femaleRatio" : 0.5
	};
	Config["Defence"] = {
		"defenceRatio" : 2.0,
		"armyCompactSize" : 700,
		"armyBreakawaySize" : 900
	};
} else if (Config.difficulty === 0)
{
	Config["Military"] = {
		"fortressStartTime" : 1500,
		"fortressLapseTime" : 1000000,
		"defenceBuildingTime" : 500,
		"advancedMilitaryStartTime" : 1300,
		"attackPlansStartTime" : 1200	// 20 minutes ought to give enough times for beginners
	};
	Config["Economy"] = {
		"townPhase" : 360,
		"cityPhase" : 840,
		"farmsteadStartTime" : 1200,
		"marketStartTime" : 1000,
		"techStartTime" : 600000,	// never
		"targetNumBuilders" : 1,
		"femaleRatio" : 0.0
	};
	Config["Defence"] = {
		"defenceRatio" : 1.0,
		"armyCompactSize" : 700,
		"armyBreakawaySize" : 900
	};
}