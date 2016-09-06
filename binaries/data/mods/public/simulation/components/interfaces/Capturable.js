Engine.RegisterInterface("Capturable");

/**
 * Message of the form { "capturePoints": number[] }
 * where "capturePoints" value is an array indexed by players id,
 * sent from Capturable component.
 */
Engine.RegisterMessageType("CapturePointsChanged");

/**
 * Message in the form of { "regenerating": boolean, "regenRate": number, "territoryDecay": number }
 * where "regenRate" value is always zero when not decaying,
 * sent from Capturable component.
 */
Engine.RegisterMessageType("CaptureRegenStateChanged");
