Modding.prototype.GetX = function() {
	return this.x * 10;
};

Engine.ReRegisterComponentType(IID_Test1, "Modding", Modding);
