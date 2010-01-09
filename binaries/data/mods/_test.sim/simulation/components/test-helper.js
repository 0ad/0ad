function TestScript1_Helper() {}

TestScript1_Helper.prototype.GetX = function() {
	return AdditionHelper(1, 2);
};

Engine.RegisterComponentType(IID_Test1, "TestScript1_Helper", TestScript1_Helper);
