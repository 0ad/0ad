Engine.RegisterInterface("Guard");

/**
 * Message of the form { "guarded": number, "data": object }
 * where "data" value is a valid MT_Attacked message,
 * sent from Guard component whenever a guarded/escorted unit is attacked.
 */
Engine.RegisterMessageType("GuardedAttacked");
