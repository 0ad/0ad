Engine.RegisterInterface("AIProxy");

/**
 * Message of the form { "id": number, "metadata": object, "owner": number }
 * sent from Commands to register metadata for buildings.
 */
Engine.RegisterMessageType("AIMetadata");
