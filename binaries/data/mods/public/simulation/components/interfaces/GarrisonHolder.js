Engine.RegisterInterface("GarrisonHolder");

/**
 * Message of the form { "added": number[], "removed": number[] }
 * sent from the GarrisonHolder component to the current entity whenever the garrisoned units change.
 */
Engine.RegisterMessageType("GarrisonedUnitsChanged");
