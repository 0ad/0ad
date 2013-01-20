Engine.RegisterInterface("BattleDetection");

// Message of the form { "to": "STATE" }.
// sent whenever the battle state changes
Engine.RegisterMessageType("BattleStateChanged");
