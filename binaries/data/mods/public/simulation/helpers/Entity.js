function DistanceBetweenEntities(first, second)
{
	var cmpFirstPosition = Engine.QueryInterface(first, IID_Position);
	if (!cmpFirstPosition || !cmpFirstPosition.IsInWorld())
			return Infinity;

	var cmpSecondPosition = Engine.QueryInterface(second, IID_Position);
	if (!cmpSecondPosition || !cmpSecondPosition.IsInWorld())
			return Infinity;

	var firstPosition = cmpFirstPosition.GetPosition2D();
	var secondPosition = cmpSecondPosition.GetPosition2D();
	return firstPosition.distanceTo(secondPosition);
}

Engine.RegisterGlobal("DistanceBetweenEntities", DistanceBetweenEntities);
