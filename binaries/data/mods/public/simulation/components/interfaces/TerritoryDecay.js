Engine.RegisterInterface("TerritoryDecay");

/**
 * Message of the form { "entity": number, "to": boolean, "rate": number }
 * sent from TerritoryDecay component whenever the decay state changes.
 */
Engine.RegisterMessageType("TerritoryDecayChanged");
