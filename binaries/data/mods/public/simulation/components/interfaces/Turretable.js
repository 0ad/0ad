Engine.RegisterInterface("Turretable");

/**
 * Message of the form { "holderID": number }
 * sent from the Turretable component whenever the turreted state changes.
 */
Engine.RegisterMessageType("TurretedStateChanged");
