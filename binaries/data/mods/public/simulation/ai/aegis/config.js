var AEGIS = function(m)
{

m.Config = function() {
	this.difficulty = 2;	// 0 is sandbox, 1 is easy, 2 is medium, 3 is hard, 4 is very hard.
				// overriden by the GUI

	this.Military = {
		"fortressLapseTime" : 540, // Time to wait between building 2 fortresses
		"defenceBuildingTime" : 600, // Time to wait before building towers or fortresses
		"attackPlansStartTime" : 0,	// time to wait before attacking. Start as soon as possible (first barracks)
		"techStartTime" : 120,	// time to wait before teching. Will only start after town phase so it's irrelevant.
		"popForBarracks1" : 15,
		"popForBarracks2" : 95,
		"timeForBlacksmith" : 900,
	};
	this.Economy = {
		"townPhase" : 180,	// time to start trying to reach town phase (might be a while after. Still need the requirements + ress )
		"cityPhase" : 840,	// time to start trying to reach city phase
		"popForMarket" : 80,
		"popForFarmstead" : 45,
		"dockStartTime" : 240,	// Time to wait before building the dock
		"techStartTime" : 0,	// time to wait before teching.
		"targetNumBuilders" : 1.5, // Base number of builders per foundation.
		"femaleRatio" : 0.4, // percent of females among the workforce.
		"initialFields" : 2
	};
	
	// Note: attack settings are set directly in attack_plan.js
	
	// defence
	this.Defence =
	{
		"defenceRatio" : 5,	// see defence.js for more info.
		"armyCompactSize" : 700,	// squared. Half-diameter of an army.
		"armyBreakawaySize" : 900  // squared.
	};
	
	// military
	this.buildings = 
	{
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
	};

	// qbot
	this.priorities = 
	{  // Note these are dynamic, you are only setting the initial values
		"house" : 350,
		"villager" : 40,
		"citizenSoldier" : 60,
		"ships" : 70,
		"economicBuilding" : 90,
		"dropsites" : 120,
		"field" : 500,
		"militaryBuilding" : 110,
		"defenceBuilding" : 70,
		"majorTech" : 700,
		"minorTech" : 50,
		"civilCentre" : 400
	};
};

//Config.prototype = new BaseConfig();

m.Config.prototype.updateDifficulty = function(difficulty)
{
	this.difficulty = difficulty;
	// changing settings based on difficulty.
	if (this.difficulty === 1)
	{
		this.Military.defenceBuildingTime = 1200;
		this.Military.attackPlansStartTime = 960;
		this.Military.popForBarracks1 = 35;
		this.Military.popForBarracks2 = 150;	// shouldn't reach it
		this.Military.popForBlacksmith = 150;	// shouldn't reach it

		this.Economy.cityPhase = 1800;
		this.Economy.popForMarket = 80;
		this.Economy.techStartTime = 600;
		this.Economy.femaleRatio = 0.6;
		this.Economy.initialFields = 1;
		// Config.Economy.targetNumWorkers will be set by AI scripts.
	}
	else if (this.difficulty === 0)
	{
		this.Military.defenceBuildingTime = 450;
		this.Military.attackPlansStartTime = 9600000;	// never
		this.Military.popForBarracks1 = 60;
		this.Military.popForBarracks2 = 150;	// shouldn't reach it
		this.Military.popForBlacksmith = 150;	// shouldn't reach it

		this.Economy.cityPhase = 240000;
		this.Economy.popForMarket = 200;
		this.Economy.techStartTime = 1800;
		this.Economy.femaleRatio = 0.2;
		this.Economy.initialFields = 1;
		// Config.Economy.targetNumWorkers will be set by AI scripts.
	}
};

return m;
}(AEGIS);
