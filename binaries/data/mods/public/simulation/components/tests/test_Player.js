Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Player.js");

Resources = {
	"GetCodes": () => ["food", "metal", "stone", "wood"],
	"GetResource": () => ({}),
};

var cmpPlayer = ConstructComponent(10, "Player", {
		"SpyCostMultiplier": 1
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
