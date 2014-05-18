var PETRA = function(m)
{

// this defines the medium difficulty
m.Config = function() {
	this.difficulty = 2;	// 0 is sandbox, 1 is easy, 2 is medium, 3 is hard, 4 is very hard.
	this.debug = 0;

	this.Military = {
		"towerLapseTime" : 90, // Time to wait between building 2 towers
		"fortressLapseTime" : 420, // Time to wait between building 2 fortresses
		"popForBarracks1" : 25,
		"popForBarracks2" : 95,
		"popForBlacksmith" : 65
	};
	this.Economy = {
		"popForTown" : 40,	// How many units we want before aging to town.
		"cityPhase" : 840,	// time to start trying to reach city phase
		"popForMarket" : 50,
		"popForDock" : 25,
		"targetNumBuilders" : 1.5, // Base number of builders per foundation.
		"targetNumTraders" : 4, // Target number of traders
		"targetNumFishers" : 1, // Target number of fishers per sea
		"femaleRatio" : 0.5, // percent of females among the workforce.
		"initialFields" : 5
	};

	// Note: attack settings are set directly in attack_plan.js
	// defense
	this.Defense =
	{
		"defenseRatio" : 2,	// see defense.js for more info.
		"armyCompactSize" : 2000,	// squared. Half-diameter of an army.
		"armyBreakawaySize" : 3500,  // squared.
		"armyMergeSize" : 1400	// squared.
	};
	
	// military
	this.buildings = 
	{
		"base" : {
			"default" : [ "structures/{civ}_civil_centre" ],
			"ptol" : [ "structures/{civ}_military_colony" ],
			"sele" : [ "structures/{civ}_military_colony" ]
		},
		"advanced" : {
			"default" : [],
			"athen" : [ "structures/{civ}_gymnasion", "structures/{civ}_prytaneion", "structures/{civ}_theatron" ],
			"brit" : [ "structures/{civ}_crannog", "structures/{civ}_rotarymill" ],
			"cart" : [ "structures/{civ}_embassy_celtic",
					"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"gaul" : [ "structures/{civ}_tavern" ],
			"hele" : [ "structures/{civ}_gymnasion", "structures/{civ}_prytaneion", "structures/{civ}_theatron" ],
			"iber" : [ "structures/{civ}_monument" ],
			"mace" : [ "structures/{civ}_siege_workshop", "structures/{civ}_library", "structures/{civ}_theatron" ],
			"maur" : [ "structures/{civ}_elephant_stables" ],
			"pers" : [ "structures/{civ}_stables", "structures/{civ}_apadana" ],
			"ptol" : [ "structures/{civ}_library" ],
			"rome" : [ "structures/{civ}_army_camp" ],
			"sele" : [ "structures/{civ}_library" ],
			"spart" : [ "structures/{civ}_syssiton", "structures/{civ}_theatron" ]
		}
	};

	this.priorities = 
	{
		"villager" : 30,	// should be slightly lower than the citizen soldier one because otherwise they get all the food
		"citizenSoldier" : 60,
		"trader" : 50,
		"ships" : 70,
		"house" : 350,
		"dropsites" : 200,
		"field" : 400,
		"economicBuilding" : 90,
		"militaryBuilding" : 130,
		"defenseBuilding" : 70,
		"civilCentre" : 950,
		"majorTech" : 700,
		"minorTech" : 40
	};

	this.personality =
	{
		"aggressive": 0.5,
		"cooperative": 0.5
	};

	this.resources = ["food", "wood", "stone", "metal"];
};

//Config.prototype = new BaseConfig();

m.Config.prototype.updateDifficulty = function(difficulty)
{
	this.difficulty = difficulty;
	// changing settings based on difficulty.
	this.targetNumTraders = 2 * this.difficulty;
	if (this.difficulty === 1)
	{
		this.Military.popForBarracks1 = 35;
		this.Military.popForBarracks2 = 150;	// shouldn't reach it
		this.Military.popForBlacksmith = 150;	// shouldn't reach it

		this.Economy.cityPhase = 1800;
		this.Economy.popForMarket = 80;
		this.Economy.femaleRatio = 0.6;
		this.Economy.initialFields = 2;
	}
	else if (this.difficulty === 0)
	{
		this.Military.popForBarracks1 = 60;
		this.Military.popForBarracks2 = 150;	// shouldn't reach it
		this.Military.popForBlacksmith = 150;	// shouldn't reach it

		this.Economy.cityPhase = 240000;
		this.Economy.popForMarket = 200;
		this.Economy.femaleRatio = 0.7;
		this.Economy.initialFields = 1;
	}
};

return m;
}(PETRA);
