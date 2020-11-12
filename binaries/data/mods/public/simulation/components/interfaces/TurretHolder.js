/**
 * Message of the form { "added": number[], "removed": number[] }
 * sent from the TurretHolder component to the current entity whenever the turrets change.
 */
Engine.RegisterMessageType("TurretsChanged");
