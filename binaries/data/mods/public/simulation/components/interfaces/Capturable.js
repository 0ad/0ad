Engine.RegisterInterface("Capturable");

// Message in the form of {"capturePoints": [gaia, p1, p2, ...]}
Engine.RegisterMessageType("CapturePointsChanged");
// Message in the form of {"regenerating": Boolean, "rate": Number}
// Where rate is always zero when not decaying
Engine.RegisterMessageType("CaptureRegenStateChanged");

