Engine.RegisterInterface("Attack");

// Message sent from Attack to the target entity, each
// time the target is damaged.
// Data: { attacker: 123, target: 234 } 
Engine.RegisterMessageType("Attacked");
