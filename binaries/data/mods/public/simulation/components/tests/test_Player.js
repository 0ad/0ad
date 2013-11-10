Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/EndGameManager.js")
Engine.LoadComponentScript("interfaces/TechnologyManager.js")
Engine.LoadComponentScript("interfaces/Timer.js")
Engine.LoadComponentScript("EndGameManager.js")
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("Timer.js")

ConstructComponent(SYSTEM_ENTITY, "EndGameManager");
ConstructComponent(SYSTEM_ENTITY, "Timer");

var cmpPlayer = ConstructComponent(10, "Player");

TS_ASSERT_EQUALS(cmpPlayer.GetPopulationCount(), 0);
TS_ASSERT_EQUALS(cmpPlayer.GetPopulationLimit(), 0);
