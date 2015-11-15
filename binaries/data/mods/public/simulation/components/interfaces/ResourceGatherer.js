Engine.RegisterInterface("ResourceGatherer");

// Message of the form { "to", [ {"type":"wood", "amount":7, "max":10} ] },
// sent whenever carrying amount changes.
Engine.RegisterMessageType("ResourceCarryingChanged");
