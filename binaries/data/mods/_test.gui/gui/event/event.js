var called1 = 0;
var called2 = 0;
var called3 = 0;
var called4 = 0;
var obj1 = Engine.GetGUIObjectByName("obj1");
var obj3 = Engine.GetGUIObjectByName("obj3");

obj1.onTick = () => { ++called1; };
Engine.GetGUIObjectByName("obj2").onTick = () => {
	++called2;
	delete obj1.onTick;
	delete obj3.onTick;
	Engine.GetGUIObjectByName("obj4").onTick = () => { ++called4; };
};
obj3.onTick = () => { ++called3; };
