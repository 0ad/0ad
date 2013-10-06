Engine.RegisterInterface("AttackDetection");

// Message of the form { "player": 1, "event": { "target": 123 , "position": { "x": 123, "z": 456 }, "time": 1, }.
// sent when a new attack is detected.
Engine.RegisterMessageType("AttackDetected");
