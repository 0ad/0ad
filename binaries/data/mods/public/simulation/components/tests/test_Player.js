Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Player.js");

var cmpPlayer = ConstructComponent(10, "Player");

TS_ASSERT_EQUALS(cmpPlayer.GetPopulationCount(), 0);
TS_ASSERT_EQUALS(cmpPlayer.GetPopulationLimit(), 0);
