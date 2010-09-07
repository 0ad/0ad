Engine.LoadComponentScript("Player.js");

var cmp = ConstructComponent(10, "Player");

TS_ASSERT_EQUALS(cmp.GetPopulationCount(), 0);
TS_ASSERT_EQUALS(cmp.GetPopulationLimit(), 0);
