Engine.RegisterInterface("ResourceGatherer");

// Message sent from ResourceGatherers to a ResourceSupply entity
// each time they gather resources from it
Engine.RegisterMessageType("ResourceGather");

// Message of the form { "to", [ {"type":"wood", "amount":7, "max":10} ] },
// sent whenever carrying amount changes.
Engine.RegisterMessageType("ResourceCarryingChanged");
