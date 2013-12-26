Engine.RegisterInterface("GarrisonHolder");

// Message of the form { } (use GetEntities if you want the current details),
// sent to the current entity whenever the garrisoned units change.
Engine.RegisterMessageType("GarrisonedUnitsChanged");

// Message of the form { "holder": this.entity, "unit" : unit } sent to the AlertRaiser
// which ordered the unit "unit" to garrison.
Engine.RegisterMessageType("UnitGarrisonedAfterAlert");
