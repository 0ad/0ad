var Config = {
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
			"cart" : [ "structures/{civ}_fortress", "structures/{civ}_embassy_celtic",
					"structures/{civ}_embassy_iberian", "structures/{civ}_embassy_italiote" ],
			"celt" : [ "structures/{civ}_kennel", "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ],
			"iber" : [ "structures/{civ}_fortress" ],
			"pers" : [ "structures/{civ}_fortress", "structures/{civ}_stables", "structures/{civ}_apadana" ]
		},
		"fort" : {
			"default" : [ "structures/{civ}_fortress" ],
			"celt" : [ "structures/{civ}_fortress_b", "structures/{civ}_fortress_g" ]
		}
	},

	"units" : {
		"citizenSoldier" : {
			"default" : [ "units/{civ}_infantry_spearman_b", "units/{civ}_infantry_slinger_b",
					"units/{civ}_infantry_swordsman_b", "units/{civ}_infantry_javelinist_b",
					"units/{civ}_infantry_archer_b" ],
			"hele" : [ "units/hele_infantry_spearman_b", "units/hele_infantry_javelinist_b",
					"units/hele_infantry_archer_b" ],
			"cart" : [ "units/cart_infantry_spearman_b", "units/cart_infantry_archer_b" ],
			"celt" : [ "units/celt_infantry_spearman_b", "units/celt_infantry_javelinist_b" ],
			"iber" : [ "units/iber_infantry_spearman_b", "units/iber_infantry_slinger_b",
					"units/iber_infantry_swordsman_b", "units/iber_infantry_javelinist_b" ],
			"pers" : [ "units/pers_infantry_spearman_b", "units/pers_infantry_archer_b",
					"units/pers_infantry_javelinist_b" ]
		},
		"advanced" : {
			"default" : [ "units/{civ}_cavalry_spearman_b", "units/{civ}_cavalry_javelinist_b",
					"units/{civ}_champion_cavalry", "units/{civ}_champion_infantry" ],
			"hele" : [ "units/hele_cavalry_swordsman_b", "units/hele_cavalry_javelinist_b",
					"units/hele_champion_cavalry_mace", "units/hele_champion_infantry_mace",
					"units/hele_champion_infantry_polis", "units/hele_champion_ranged_polis",
					"units/thebes_sacred_band_hoplitai", "units/thespian_melanochitones",
					"units/sparta_hellenistic_phalangitai", "units/thrace_black_cloak" ],
			"cart" : [ "units/cart_cavalry_javelinist_b", "units/cart_champion_cavalry",
					"units/cart_infantry_swordsman_2_b", "units/cart_cavalry_spearman_b",
					"units/cart_infantry_javelinist_b", "units/cart_infantry_slinger_b",
					"units/cart_cavalry_swordsman_b", "units/cart_infantry_swordsman_b",
					"units/cart_cavalry_swordsman_2_b", "units/cart_sacred_band_cavalry" ],
			"celt" : [ "units/celt_cavalry_javelinist_b", "units/celt_cavalry_swordsman_b",
					"units/celt_champion_cavalry_gaul", "units/celt_champion_infantry_gaul",
					"units/celt_champion_cavalry_brit", "units/celt_champion_infantry_brit", "units/celt_fanatic" ],
			"iber" : [ "units/iber_cavalry_spearman_b", "units/iber_champion_cavalry", "units/iber_champion_infantry" ],
			"pers" : [ "units/pers_cavalry_javelinist_b", "units/pers_champion_infantry",
					"units/pers_champion_cavalry", "units/pers_cavalry_spearman_b", "units/pers_cavalry_swordsman_b",
					"units/pers_cavalry_javelinist_b", "units/pers_cavalry_archer_b", "units/pers_kardakes_hoplite",
					"units/pers_kardakes_skirmisher", "units/pers_war_elephant" ]
		},
		"siege" : {
			"default" : [ "units/{civ}_mechanical_siege_oxybeles", "units/{civ}_mechanical_siege_lithobolos",
					"units/{civ}_mechanical_siege_ballista", "units/{civ}_mechanical_siege_ram" ]
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
