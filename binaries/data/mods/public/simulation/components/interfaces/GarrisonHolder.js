Engine.RegisterInterface("GarrisonHolder");

/**
 * Message of the form { "added": number[], "removed": number[] }
 * sent from the GarrisonHolder component to the current entity whenever the garrisoned units change.
 */
Engine.RegisterMessageType("GarrisonedUnitsChanged");

/**
 * Message of the form { "holder": number, "unit" : number }
 * sent to the AlertRaiser which ordered the specified unit to garrison.
 */
Engine.RegisterMessageType("UnitGarrisonedAfterAlert");
