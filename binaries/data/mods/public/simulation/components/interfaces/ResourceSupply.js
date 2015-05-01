Engine.RegisterInterface("ResourceSupply");

// Message of the form { "from": 100, "to", 90 },
// sent whenever supply level changes.
Engine.RegisterMessageType("ResourceSupplyChanged");

// Message of the form { "to": 10 },
// sent whenever the number of gatherer changes
Engine.RegisterMessageType("ResourceSupplyNumGatherersChanged");
