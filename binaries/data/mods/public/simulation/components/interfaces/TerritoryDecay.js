Engine.RegisterInterface("TerritoryDecay");

// Message of the form { "to": decaying }.
// sent whenever the decay state changes
Engine.RegisterMessageType("TerritoryDecayChanged");
