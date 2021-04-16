Engine.RegisterInterface("Resistance");

/**
 * Message of the form { "entity": entity, "invulnerability": true/false }
 */
Engine.RegisterMessageType("InvulnerabilityChanged");

/**
 * Message of the form {
 *		"type": string,
 *		"target": number,
 *		"attacker": attacker,
 *		"attackerOwner": number,
 *		"damage": number,
 *		"capture": number,
 *		"statusEffects": Object[],
 *		"fromStatusEffect": boolean }
 * sent from Attack.js-helper to the target entity, each time the target is damaged.
 */
Engine.RegisterMessageType("Attacked");
