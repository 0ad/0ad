function TestScript1_DestroyEntity() {}

TestScript1_DestroyEntity.prototype.GetX = function()
{
	Engine.DestroyEntity(10);
	return 0;
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_DestroyEntity", TestScript1_DestroyEntity);
