Engine.RegisterInterface("TechnologyManager");

// Message of the form { "component": "Attack", "player": 3 }
// Sent when a new technology is researched which modifies a component
Engine.RegisterMessageType("TechnologyModificationChange");
