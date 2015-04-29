Engine.RegisterInterface("TerritoryDecay");

// Message of the form { "entity": this.entity, "to": decaying }.
// sent whenever the decay state changes
Engine.RegisterMessageType("TerritoryDecayChanged");
