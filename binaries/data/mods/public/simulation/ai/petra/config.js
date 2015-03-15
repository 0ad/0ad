var PETRA = function(m)
{

m.Config = function(difficulty)
{
	// 0 is sandbox, 1 is very easy, 2 is easy, 3 is medium, 4 is hard and 5 is very hard.
	this.difficulty = (difficulty !== undefined) ? difficulty : 3;

	// debug level: 0=none, 1=sanity checks, 2=debug, 3=detailed debug, -100=serializatio debug
	this.debug = 0;

	this.chat = true;   // false to prevent AI's chats

	this.popScaling = 1;  // scale factor depending on the max population

	this.Military = {
		"towerLapseTime" : 90, // Time to wait between building 2 towers
		"fortressLapseTime" : 390, // Time to wait between building 2 fortresses
		"popForBarracks1" : 25,
		"popForBarracks2" : 95,
		"popForBlacksmith" : 65
	};
	this.Economy = {
		"popForTown" : 40,	// How many units we want before aging to town.
		"cityPhase" : 840,	// time to start trying to reach city phase
		"popForMarket" : 50,
		"popForDock" : 25,
		"targetNumTraders" : 5, // Target number of traders
		"targetNumFishers" : 1, // Target number of fishers per sea
		"femaleRatio" : 0.5, // fraction of females among the workforce
		"provisionFields" : 2
	};

	this.distUnitGain = 110*110;   // TODO  take it directly from helpers/TraderGain.js

	// Note: attack settings are set directly in attack_plan.js
	// defense
	this.Defense =
	{
		"defenseRatio" : 2,	// ratio of defenders/attackers.
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
			"brit" : [ "structures/{civ}_rotarymill" ],
			"cart" : [ "structures/{civ}_embassy_celtic",
				"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"gaul" : [ "structures/{civ}_tavern" ],
			"iber" : [ "structures/{civ}_monument" ],
			"mace" : [ "structures/{civ}_siege_workshop", "structures/{civ}_library", "structures/{civ}_theatron" ],
			"maur" : [ "structures/{civ}_elephant_stables" ],
			"pers" : [ "structures/{civ}_stables", "structures/{civ}_apadana" ],
			"ptol" : [ "structures/{civ}_library" ],
			"rome" : [ "structures/{civ}_army_camp" ],
			"sele" : [ "structures/{civ}_library" ],
			"spart" : [ "structures/{civ}_syssiton", "structures/{civ}_theatron" ]
		},
		"naval" : {
			"default" : [],
//			"brit" : [ "structures/{civ}_crannog" ],
			"cart" : [ "structures/{civ}_super_dock" ]
		}
	};

	this.priorities = 
	{
		"villager" : 30,      // should be slightly lower than the citizen soldier one to not get all the food
		"citizenSoldier" : 60,
		"trader" : 50,
		"ships" : 70,
		"house" : 350,
		"dropsites" : 200,
		"field" : 400,
		"dock" : 90,
		"economicBuilding" : 90,
		"militaryBuilding" : 130,
		"defenseBuilding" : 70,
		"civilCentre" : 950,
		"majorTech" : 700,
		"minorTech" : 40,
		"emergency" : 1000    // used only in emergency situations, should be the highest one 
	};

	this.personality =
	{
		"aggressive": 0.5,
		"cooperative": 0.5,
		"defensive": 0.5
	};

	this.resources = ["food", "wood", "stone", "metal"];
};

m.Config.prototype.setConfig = function(gameState)
{
	// initialize personality traits
	if (this.difficulty > 1)
	{
		this.personality.aggressive = Math.random();
		this.personality.cooperative = Math.random();
		this.personality.defensive = Math.random();
	}
	else
		this.personality.aggressive = 0.1;

	// changing settings based on difficulty or personality
	if (this.difficulty < 2)
	{
		this.Military.popForBarracks1 = 60;
		this.Military.popForBarracks2 = 300;
		this.Military.popForBlacksmith = 300;

		this.Economy.cityPhase = 240000;
		this.Economy.popForMarket = 200;
		this.Economy.femaleRatio = 0.7;
		this.Economy.provisionFields = 1;
	}
	else if (this.difficulty < 3)
	{
		this.Military.popForBarracks1 = 35;
		this.Military.popForBarracks2 = 150;
		this.Military.popForBlacksmith = 150;

		this.Economy.cityPhase = 1800;
		this.Economy.popForMarket = 80;
		this.Economy.femaleRatio = 0.6;
		this.Economy.provisionFields = 1;
	}
	else
	{
		this.Military.towerLapseTime += Math.round(20*(this.personality.defensive - 0.5));
		this.Military.fortressLapseTime += Math.round(60*(this.personality.defensive - 0.5));

		if (this.personality.aggressive > 0.7)
		{
			this.Military.popForBarracks1 = 12;
			this.Economy.popForTown = 55;
			this.Economy.popForMarket = 60;
			this.Economy.femaleRatio = 0.3;
			this.priorities.defenseBuilding = 60;
		}
	}

	this.Economy.targetNumTraders = 2 + this.difficulty;

	if (gameState.getPopulationMax() < 300)
		this.popScaling = Math.sqrt(gameState.getPopulationMax() / 300);

	if (this.debug < 2)
		return;
	API3.warn(" >>>  Petra bot: personality = " + uneval(this.personality));
};

m.Config.prototype.Serialize = function()
{
	var data = {};
	for (let key in this)
		if (this.hasOwnProperty(key) && key != "debug")
			data[key] = this[key];
	return data;
};

m.Config.prototype.Deserialize = function(data)
{
	for (let key in data)
		this[key] = data[key];
};

return m;
}(PETRA);
