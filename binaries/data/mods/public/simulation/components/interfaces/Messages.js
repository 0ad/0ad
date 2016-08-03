/**
 * Message of the form { "entity": number, "newentity": number }
 * sent when one entity is changed to another:
 * - from Foundation component when a building construction is done
 * - from Formation component
 * - from Health component when an entity died and should remain as a resource
 * - from Promotion component when a unit is promoted
 * - from Mirage component when a fogged entity is re-discovered
 * - from SkirmishReplacer component when a skirmish entity has been replaced
 * - from Transform helper when an entity has been upgraded
 */
Engine.RegisterMessageType("EntityRenamed");

/**
 * Message of the form { "player": number, "otherPlayer": number }
 * sent from Player component when diplomacy changed for one player or between two players.
 */
Engine.RegisterMessageType("DiplomacyChanged");

/**
 * Message of the form { "to": number, "from": number, "amounts": object }
 * sent from Player component whenever a tribute is sent.
 */
Engine.RegisterMessageType("TributeExchanged");

/**
 * Message of the form {}
 * sent from InitGame for component map-dependent initialization.
 */
Engine.RegisterMessageType("InitGame");
