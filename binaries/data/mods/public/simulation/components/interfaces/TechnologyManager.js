Engine.RegisterInterface("TechnologyManager");

/**
 * Message of the form { "player": number, "tech": string }
 * sent from TechnologyManager component whenever a technology research is finished.
 */
Engine.RegisterMessageType("ResearchFinished");
