from .RLAPI_pb2 import Actions, Action, ResetRequest
from .RLAPI_pb2_grpc import RLAPIStub
import grpc
import json
import math
import gym

class ZeroAD():
    def __init__(self, uri='localhost:50051'):
        # TODO: If uri is none, spin up an instance ourselves!
        channel = grpc.insecure_channel(uri)
        self.stub = RLAPIStub(channel)
        self.current_state = None

    def step(self, actions):
        # TODO: Add player ids?
        cmds = Actions()
        cmds.actions.extend([
            Action(content=json.dumps(a)) for a in actions
        ])
        res = self.stub.Step(cmds)
        self.current_state = GameState(res.content)
        return self.current_state

    def reset(self, config=None):
        req = ResetRequest(scenario=config)
        res = self.stub.Reset(req)
        self.current_state = GameState(res.content)
        return self.current_state

class GameState():
    def __init__(self, txt):
        self.data = json.loads(txt)

    def units(self, owner=None, type=None):
        filter_fn = lambda e: (owner is None or e['owner'] == owner) and \
                (type is None or type in e['template'])
        return [ e for e in self.data['entities'].values() if filter_fn(e) ]

    def center(self, units=None):
        if units is None:
            units = self.units(owner=1)

        positions = [ unit['position'] for unit in units ]
        squad_center = [
            sum([ x for [x, z] in positions ])/len(positions),
            sum([ z for [x, z] in positions ])/len(positions)
        ]
        return squad_center

    def closest(self, units, position=None):
        if position is None:
            position = self.center()

        min_dist = math.inf
        closest = None
        for unit in units:
            dist = self.dist(unit['position'], position)
            if dist < min_dist:
                min_dist = dist
                closest = unit

        return closest

    def offset(self, p1, p2):
        [x, z] = p1
        [x2, z2] = p2
        dx = x2 - x
        dz = z2 - z
        return [ dx, dz ]

    def magnitude(self, vec):
        [x, z] = vec
        return math.sqrt(math.pow(x, 2) + math.pow(z, 2))

    def dist(self, p1, p2):
        return self.magnitude(self.offset(p1, p2))

