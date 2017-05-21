Resources = {
	"GetCodes": () => ["food", "metal", "stone", "wood"],
	"GetResource": () => ({}),
	"BuildSchema": (type) => {
		let schema = "";
		for (let res of Resources.GetCodes())
			schema +=
				"<optional>" +
					"<element name='" + res + "'>" +
						"<ref name='" + type + "'/>" +
					"</element>" +
				"</optional>";
		return "<interleave>" + schema + "</interleave>";
	}
};

Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Player.js");

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"GetTemplate": name => null,
	"GetCurrentTemplateName" : ent => null
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => null,
});

var cmpPlayer = ConstructComponent(10, "Player", {
	"SpyCostMultiplier": 1,
	"BarterMultiplier": {
		"Buy": {
			"wood": 1.0,
			"stone": 1.0,
			"metal": 1.0
		},
		"Sell": {
			"wood": 1.0,
			"stone": 1.0,
			"metal": 1.0
		}
	},
});

TS_ASSERT_EQUALS(cmpPlayer.GetPopulationCount(), 0);
TS_ASSERT_EQUALS(cmpPlayer.GetPopulationLimit(), 0);

cmpPlayer.SetDiplomacy([-1, 1, 0, 1, -1]);
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetAllies(), [1, 3]);
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetEnemies(), [0, 4]);

var diplo = cmpPlayer.GetDiplomacy();
diplo[0] = 1;
TS_ASSERT(cmpPlayer.IsEnemy(0));

diplo = [1, 1, 0];
cmpPlayer.SetDiplomacy(diplo);
diplo[1] = -1;
TS_ASSERT(cmpPlayer.IsAlly(1));

TS_ASSERT_EQUALS(cmpPlayer.GetSpyCostMultiplier(), 1);
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetBarterMultiplier(), {
	"buy": {
		"wood": 1.0,
		"stone": 1.0,
		"metal": 1.0
	},
	"sell": {
		"wood": 1.0,
		"stone": 1.0,
		"metal": 1.0
	}
});
