Engine.LoadComponentScript("GuiInterface.js");

var cmp = ConstructComponent(SYSTEM_ENTITY, "GuiInterface");

TS_ASSERT_UNEVAL_EQUALS(cmp.GetSimulationState(), { test: "simulation state" });

AddMock(10, IID_Position, {
	GetPosition: function() {
		return {x:1, y:2, z:3};
	},
});

var state = cmp.GetEntityState(-1, 10);
TS_ASSERT_UNEVAL_EQUALS(state, { position: {x:1, y:2, z:3} });
