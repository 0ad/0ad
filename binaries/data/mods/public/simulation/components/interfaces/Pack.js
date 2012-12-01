Engine.RegisterInterface("Pack");

// Message of the form { "progress": 0.5 },
// sent whenever packing progress is updated.
Engine.RegisterMessageType("PackProgressUpdate");

// Message of the form { "packed": true },
// sent whenever packing finishes.
Engine.RegisterMessageType("PackFinished");
