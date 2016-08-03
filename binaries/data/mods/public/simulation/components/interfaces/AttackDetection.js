Engine.RegisterInterface("AttackDetection");

/**
 * Message of the form { "player": number, "event": object }
 * where "event" value is an object of the form
 * { "target": number , "position": { "x": number, "z": number }, "time": number, "targetIsDomesticAnimal": boolean }
 * sent from AttackDetection component when a new attack is detected.
 */
Engine.RegisterMessageType("AttackDetected");
