function ProcessCommand(player, cmd)
{
//	print("command: " + player + " " + uneval(cmd) + "\n");
	
	switch (cmd.type)
	{
	case "spin":
		for each (var ent in cmd.entities)
		{
			var pos = Engine.QueryInterface(ent, IID_Position);
			if (! pos)
				continue;
			pos.SetYRotation(pos.GetRotation().y + 1);
		}
		break;

	case "walk":
		for each (var ent in cmd.entities)
		{
			var motion = Engine.QueryInterface(ent, IID_UnitMotion);
			if (! motion)
				continue;
			motion.MoveToPoint(cmd.x, cmd.z);
		}
		break;

	case "construct":
		// TODO: this should do all sorts of stuff with foundations and resource costs etc
		var ent = Engine.AddEntity(cmd.template);
		if (ent)
		{
			var pos = Engine.QueryInterface(ent, IID_Position);
			if (pos)
			{
				pos.JumpTo(cmd.x, cmd.z);
				pos.SetYRotation(cmd.angle);
			}
		}
		break;

	default:
		print("Ignoring unrecognised command type '" + cmd.type + "'\n");
	}
}

Engine.RegisterGlobal("ProcessCommand", ProcessCommand);
