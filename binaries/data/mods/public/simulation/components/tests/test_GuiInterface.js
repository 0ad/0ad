Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("GuiInterface.js");

var cmp = ConstructComponent(SYSTEM_ENTITY, "GuiInterface");

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	GetNumPlayers: function() { return 2; },
	GetPlayerByID: function(id) { TS_ASSERT(id === 0 || id === 1); return 100+id; }
});

AddMock(100, IID_Player, {
	GetPopulationCount: function() { return 10; },
	GetPopulationLimit: function() { return 20; }
});

AddMock(101, IID_Player, {
	GetPopulationCount: function() { return 40; },
	GetPopulationLimit: function() { return 30; }
});

TS_ASSERT_UNEVAL_EQUALS(cmp.GetSimulationState(), { players: [{popCount:10, popLimit:20}, {popCount:40, popLimit:30}] });


AddMock(10, IID_Position, {
	GetPosition: function() {
		return {x:1, y:2, z:3};
	}
});

AddMock(10, IID_Builder, {
	GetEntitiesList: function() {
		return ["test1", "test2"];
	}
});

var state = cmp.GetEntityState(-1, 10);
TS_ASSERT_UNEVAL_EQUALS(state, { position: {x:1, y:2, z:3}, buildEntities: ["test1", "test2"] });
