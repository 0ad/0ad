/**
 * Broadcast message 
 * sent when one entity is changed to other:
 * from Foundation component when building constuction is done
 * and from Promotion component when unit is promoted
 * Data: { entity: <integer>, newentity: <integer> }
 */
Engine.RegisterMessageType("EntityRenamed");

