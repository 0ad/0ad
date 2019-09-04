Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Resources = {
	"BuildSchema": type => {
		let schema = "";
		for (let res of ["food", "metal", "stone", "wood"])
			schema +=
				"<optional>" +
					"<element name='" + res + "'>" +
						"<ref name='" + type + "'/>" +
					"</element>" +
				"</optional>";
		return "<interleave>" + schema + "</interleave>";
	}
};
Engine.LoadComponentScript("interfaces/ModifiersManager.js"); // Provides `IID_ModifiersManager`, used below.
Engine.LoadComponentScript("interfaces/Timer.js"); // Provides `IID_Timer`, used below.

// What we're testing:
Engine.LoadComponentScript("interfaces/Upgrade.js");
Engine.LoadComponentScript("Upgrade.js");

// Input (bare minimum needed for tests):
let techs = {
	"alter_tower_upgrade_cost": {
		"modifications": [
			{ "value": "Upgrade/Cost/stone", "add": 60.0 },
			{ "value": "Upgrade/Cost/wood", "multiply": 0.5 },
			{ "value": "Upgrade/Time", "replace": 90 }
		],
		"affects": ["Tower"]
	}
};
let template = {
	"Identity": {
		"Classes": { '@datatype': "tokens", "_string": "Tower" },
		"VisibleClasses": { '@datatype': "tokens", "_string": "" }
	},
	"Upgrade": {
		"Tower": {
			"Cost": { "stone": "100", "wood": "50" },
			"Entity": "structures/{civ}_defense_tower",
			"Time": "100"
		}
	}
};
let civCode = "pony";
let playerID = 1;

// Usually, the tech modifications would be worked out by the TechnologyManager
// with assistance from globalscripts. This test is not about testing the
// TechnologyManager, so the modifications (both with and without the technology
// researched) are worked out before hand and placed here.
let isResearched = false;
let templateTechModifications = {
	"without": {},
	"with": {
		"Upgrade/Cost/stone": [{ "affects": [["Tower"]], "add": 60 }],
		"Upgrade/Cost/wood": [{ "affects": [["Tower"]], "multiply": 0.5 }],
		"Upgrade/Time": [{ "affects": [["Tower"]], "replace": 90 }]
	}
};
let entityTechModifications = {
	"without": {
		'Upgrade/Cost/stone': { "20": { "origValue": 100, "newValue": 100 } },
		'Upgrade/Cost/wood': { "20": { "origValue": 50, "newValue": 50 } },
		'Upgrade/Time': { "20": { "origValue": 100, "newValue": 100 } }
	},
	"with": {
		'Upgrade/Cost/stone': { "20": { "origValue": 100, "newValue": 160 } },
		'Upgrade/Cost/wood': { "20": { "origValue": 50, "newValue": 25 } },
		'Upgrade/Time': { "20": { "origValue": 100, "newValue": 90 } }
	}
};

/**
 * Initialise various bits.
 */
// System Entities:
AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": pID => 10 // Called in helpers/player.js::QueryPlayerIDInterface(), as part of Tests T2 and T5.
});
AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": () => template // Called in components/Upgrade.js::ChangeUpgradedEntityCount().
});
AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetInterval": () => 1, // Called in components/Upgrade.js::Upgrade().
	"CancelTimer": () => {} // Called in components/Upgrade.js::CancelUpgrade().
});

AddMock(SYSTEM_ENTITY, IID_ModifiersManager, {
	"ApplyTemplateModifiers": (valueName, curValue, template, player) => {
		// Called in helpers/ValueModification.js::ApplyValueModificationsToTemplate()
		// as part of Tests T2 and T5 below.
		let mods = isResearched ? templateTechModifications.with : templateTechModifications.without;

		if (mods[valueName])
			return GetTechModifiedProperty(mods[valueName], GetIdentityClasses(template.Identity), curValue);
		return curValue;
	},
	"ApplyModifiers": (valueName, curValue, ent) => {
		// Called in helpers/ValueModification.js::ApplyValueModificationsToEntity()
		// as part of Tests T3, T6 and T7 below.
		let mods = isResearched ? entityTechModifications.with : entityTechModifications.without;
		return mods[valueName][ent].newValue;
	}
});

// Init Player:
AddMock(10, IID_Player, {
	"AddResources": () => {}, // Called in components/Upgrade.js::CancelUpgrade().
	"GetPlayerID": () => playerID, // Called in helpers/Player.js::QueryOwnerInterface() (and several times below).
	"GetTimeMultiplier": () => 1.0, // Called in components/Upgrade.js::GetUpgradeTime().
	"TrySubtractResources": () => true // Called in components/Upgrade.js::Upgrade().
});

// Create an entity with an Upgrade component:
AddMock(20, IID_Ownership, {
	"GetOwner": () => playerID // Called in helpers/Player.js::QueryOwnerInterface().
});
AddMock(20, IID_Identity, {
	"GetCiv": () => civCode // Called in components/Upgrade.js::init().
});
let cmpUpgrade = ConstructComponent(20, "Upgrade", template.Upgrade);
cmpUpgrade.owner = playerID;

/**
 * Now to start the test proper
 * To start with, no techs are researched...
 */
// T1: Check the cost of the upgrade without a player value being passed (as it would be in the structree).
let parsed_template = GetTemplateDataHelper(template, null, {});
TS_ASSERT_UNEVAL_EQUALS(parsed_template.upgrades[0].cost, { "stone": 100, "wood": 50, "time": 100 });

// T2: Check the value, with a player ID (as it would be in-session).
parsed_template = GetTemplateDataHelper(template, playerID, {});
TS_ASSERT_UNEVAL_EQUALS(parsed_template.upgrades[0].cost, { "stone": 100, "wood": 50, "time": 100 });

// T3: Check that the value is correct within the Update Component.
TS_ASSERT_UNEVAL_EQUALS(cmpUpgrade.GetUpgrades()[0].cost, { "stone": 100, "wood": 50, "time": 100 });

/**
 * Tell the Upgrade component to start the Upgrade,
 * then mark the technology that alters the upgrade cost as researched.
 */
cmpUpgrade.Upgrade("structures/"+civCode+"_defense_tower");
isResearched = true;

// T4: Check that the player-less value hasn't increased...
parsed_template = GetTemplateDataHelper(template, null, {});
TS_ASSERT_UNEVAL_EQUALS(parsed_template.upgrades[0].cost, { "stone": 100, "wood": 50, "time": 100 });

// T5: ...but the player-backed value has.
parsed_template = GetTemplateDataHelper(template, playerID, {});
TS_ASSERT_UNEVAL_EQUALS(parsed_template.upgrades[0].cost, { "stone": 160, "wood": 25, "time": 90 });

// T6: The upgrade component should still be using the old resource cost (but new time cost) for the upgrade in progress...
TS_ASSERT_UNEVAL_EQUALS(cmpUpgrade.GetUpgrades()[0].cost, { "stone": 100, "wood": 50, "time": 90 });

// T7: ...but with the upgrade cancelled, it now uses the modified value.
cmpUpgrade.CancelUpgrade(playerID);
TS_ASSERT_UNEVAL_EQUALS(cmpUpgrade.GetUpgrades()[0].cost, { "stone": 160, "wood": 25, "time": 90 });
