Engine.RegisterInterface("Attack");

/**
 * Message of the form { "attacker": number, "target": number, "type": string, "damage": number }
 * sent from Attack component and by Damage helper to the target entity, each time the target is attacked or damaged.
 */
Engine.RegisterMessageType("Attacked");
