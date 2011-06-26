function DistanceBetweenEntities(first, second)
{
	var cmpFirstPosition = Engine.QueryInterface(first, IID_Position);
	if (!cmpFirstPosition || !cmpFirstPosition.IsInWorld())
			return Infinity;
	var firstPosition = cmpFirstPosition.GetPosition();

	var cmpSecondPosition = Engine.QueryInterface(second, IID_Position);
	if (!cmpSecondPosition || !cmpSecondPosition.IsInWorld())
			return Infinity;
	var secondPosition = cmpSecondPosition.GetPosition();

  var dx = secondPosition.x - firstPosition.x;
  var dz = secondPosition.z - firstPosition.z;

  var horizDistance = Math.sqrt(dx * dx + dz * dz);
  return horizDistance;
}

/**
 *  Returns entities ordered by decreasing priority
 *  Do not alter order when units have the same priority
 */
function SortEntitiesByPriority(ents)
{
  // Priority list, weakers first
  var types = ["Structure", "Worker"];

  ents.sort(function  (a, b) {
    var cmpIdentityA = Engine.QueryInterface(a, IID_Identity);
    var cmpIdentityB = Engine.QueryInterface(b, IID_Identity);
    if (!cmpIdentityA || !cmpIdentityB)
      return 0;
    var classesA = cmpIdentityA.GetClassesList();
    var classesB = cmpIdentityB.GetClassesList();
    for each (var type in types)
    {
      var inA = classesA.indexOf(type) != -1;
      var inB = classesB.indexOf(type) != -1;
      if (inA && !inB)
        return +1;
      if (inB && !inA)
        return -1;
    }
    return 0;
  });
}

Engine.RegisterGlobal("DistanceBetweenEntities", DistanceBetweenEntities);
Engine.RegisterGlobal("SortEntitiesByPriority", SortEntitiesByPriority);
