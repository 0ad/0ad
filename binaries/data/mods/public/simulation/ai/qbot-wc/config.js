// Baseconfig is the highest difficulty.
var baseConfig = {
	"Military" : {
		"fortressStartTime" : 780, // Time to wait before building one fortress.
		"fortressLapseTime" : 600, // Time to wait between building 2 fortresses (minimal)
		"defenceBuildingTime" : 600, // Time to wait before building towers or fortresses
		"advancedMilitaryStartTime" : 720, // Time to wait before building advanced military buildings. Also limited by phase 2.
		"attackPlansStartTime" : 0	// time to wait before attacking. Start as soon as possible (first barracks)
	},
	"Economy" : {
		"townPhase" : 230,	// time to start trying to reach town phase (might be a while after. Still need the requirements + ress )
		"cityPhase" : 720,	// time to start trying to reach city phase
		"farmsteadStartTime" : 400,	// Time to wait before building a farmstead.
		"marketStartTime" : 480, // Time to wait before building the market.
		"dockStartTime" : 240,	// Time to wait before building the dock
		"techStartTime" : 600,	// time to wait before teching.
		"targetNumBuilders" : 1.5, // Base number of builders per foundation. Later updated, but this remains a multiplier.
		"femaleRatio" : 0.4 // percent of females among the workforce.
	},
	
	// Note: attack settings are set directly in attack_plan.js
	
	// defence
	"Defence" : {
		"defenceRatio" : 2,	// see defence.js for more info.
		"armyCompactSize" : 700,	// squared. Half-diameter of an army.
		"armyBreakawaySize" : 900  // squared.
	},
	
	// military
	"buildings" : {
		"moderate" : {
			"default" : [ "structures/{civ}_barracks" ]
		},
		"advanced" : {
			"default" : [],
			"hele" : [ "structures/{civ}_gymnasion" ],
			"athen" : [ "structures/{civ}_gymnasion" ],
			"spart" : [ "structures/{civ}_syssiton" ],
			"cart" : [ "structures/{civ}_embassy_celtic",
					"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"celt" : [ "structures/{civ}_kennel" ],
			"pers" : [ "structures/{civ}_fortress", "structures/{civ}_stables", "structures/{civ}_apadana" ],
			"rome" : [ "structures/{civ}_army_camp" ],
			"maur" : [ "structures/{civ}_elephant_stables"]
		},
		"fort" : {
			"default" : [ "structures/{civ}_fortress" ],
			"celt" : [ "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ]
		}
	},

	// qbot
	"priorities" : {  // Note these are dynamic, you are only setting the initial values
		"house" : 200,
		"citizenSoldier" : 50,
		"villager" : 60,
		"economicBuilding" : 70,
		"dropsites" : 160,
		"field" : 1000,
		"militaryBuilding" : 90,
		"defenceBuilding" : 20,
		"majorTech" : 250,
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
		"fortressLapseTime" : 900,
		"defenceBuildingTime" : 720,
		"advancedMilitaryStartTime" : 1000,
		"attackPlansStartTime" : 600
	};
	Config["Economy"] = {
		"townPhase" : 300,
		"cityPhase" : 900,
		"farmsteadStartTime" : 600,
		"marketStartTime" : 800,
		"techStartTime" : 1320,
		"targetNumBuilders" : 2,
		"femaleRatio" : 0.5
	};
	Config["Defence"] = {
		"armyCompactSize" : 700,
		"armyBreakawaySize" : 900
	};
} else if (Config.difficulty === 0)
{
	Config["Military"] = {
		"fortressStartTime" : 1500,
		"fortressLapseTime" : 1000000,
		"defenceBuildingTime" : 900,
		"advancedMilitaryStartTime" : 1300,
		"attackPlansStartTime" : 1200	// 20 minutes ought to give enough times for beginners
	};
	Config["Economy"] = {
		"townPhase" : 480,
		"cityPhase" : 1200,
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