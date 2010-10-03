Engine.RegisterInterface("ResourceSupply");

// Message of the form { "from": 100, "to", 90 },
// sent whenever supply level changes.
Engine.RegisterMessageType("ResourceSupplyChanged");
