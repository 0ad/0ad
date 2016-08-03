Engine.RegisterInterface("Pack");

/**
 * Message of the form { "progress": number }
 * sent from Pack component whenever packing progress is updated.
 */
Engine.RegisterMessageType("PackProgressUpdate");

/**
 * Message of the form { "packed": boolean }
 * sent from Pack component whenever the unit has become packed.
 */
Engine.RegisterMessageType("PackFinished");
