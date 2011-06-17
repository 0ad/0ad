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

Engine.RegisterGlobal("DistanceBetweenEntities", DistanceBetweenEntities);
