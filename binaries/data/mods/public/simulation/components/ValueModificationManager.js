function ValueModificationManager() {};

/*
 * A component to give the C++ defined components access to all value modifying components 
 * via the helper script.
 */
ValueModificationManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

ValueModificationManager.prototype.ApplyModifications = function(valueName, currentValue, entity)
{
	return ApplyValueModificationsToEntity(valueName, currentValue, entity);
};

Engine.RegisterSystemComponentType(IID_ValueModificationManager, "ValueModificationManager", ValueModificationManager);

