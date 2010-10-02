Engine.RegisterInterface("Health");

// Message of the form { "from": 100, "to", 90 },
// sent whenever health changes.
Engine.RegisterMessageType("HealthChanged");
