Engine.RegisterInterface("UnitAI");

// Message of the form { "idle": true },
// sent whenever the unit's idle status changes.
Engine.RegisterMessageType("UnitIdleChanged");
