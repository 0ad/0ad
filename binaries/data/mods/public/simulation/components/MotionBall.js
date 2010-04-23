function MotionBallScripted() {}

MotionBallScripted.prototype.Schema =
	"<a:component type='test'/><empty/>";

MotionBallScripted.prototype.Init = function() {
	this.speedX = 0;
	this.speedZ = 0;
};

MotionBallScripted.prototype.OnUpdate = function(msg) {
	var dt = msg.turnLength;

	var cmpPos = Engine.QueryInterface(this.entity, IID_Position);
	var cmpTerrain = Engine.QueryInterface(SYSTEM_ENTITY, IID_Terrain);

	var pos = cmpPos.GetPosition();

	var normal = cmpTerrain.CalcNormal(pos.x, pos.z);

	var g = 10;
	var forceX = normal.x * g;
	var forceZ = normal.z * g;

	this.speedX += forceX * dt;
	this.speedZ += forceZ * dt;

	var drag = 0.5; // fractional decay per second
	this.speedX *= Math.pow(drag, dt);
	this.speedZ *= Math.pow(drag, dt);

	cmpPos.MoveTo(pos.x + this.speedX * dt, pos.z + this.speedZ * dt);
};

Engine.RegisterComponentType(IID_Motion, "MotionBallScripted", MotionBallScripted);
