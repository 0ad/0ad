Engine.RegisterInterface("UnitAI");

// Message of the form { "idle": true },
// sent whenever the unit's idle status changes.
Engine.RegisterMessageType("UnitIdleChanged");

// Message of the form { "to": "STATE.NAME" }.
// sent whenever the unit changes state
Engine.RegisterMessageType("UnitAIStateChanged");

// Message of the form { "to": orderData }.
// sent whenever the unit changes state
Engine.RegisterMessageType("UnitAIOrderDataChanged");

// Message of the form { "entity": entity },
// sent whenever a pickup is requested
Engine.RegisterMessageType("PickupRequested");

// Message of the form { "entity": entity },
// sent whenever a pickup is no more needed
Engine.RegisterMessageType("PickupCanceled");
