function TestScript1_AddEntity() {}

TestScript1_AddEntity.prototype.GetX = function()
{
	if (Engine.AddEntity("bogus-template-name") !== 0)
		throw new Error("bogus AddEntity failed");

	return Engine.AddEntity("test1");
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_AddEntity", TestScript1_AddEntity);
