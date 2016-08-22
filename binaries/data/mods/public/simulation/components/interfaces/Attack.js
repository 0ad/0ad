Engine.RegisterInterface("Attack");

/**
 * Message of the form { "attacker": number, "target": number, "type": string, "damage": number, "attackerOwner": number }
 * sent from Attack component and by Damage component to the target entity, each time the target is attacked or damaged.
 */
Engine.RegisterMessageType("Attacked");
