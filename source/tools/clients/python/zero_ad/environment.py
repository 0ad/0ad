from .RLAPI_pb2 import Actions, Action, ResetRequest, GetTemplateRequest
from .RLAPI_pb2_grpc import RLAPIStub
import grpc
import json
import math
import gym
from xml.etree import ElementTree
from itertools import cycle

class ZeroAD():
    def __init__(self, uri='localhost:50050'):
        channel = grpc.insecure_channel(uri)
        self.stub = RLAPIStub(channel)
        self.current_state = None
        self.cache = {}
        self.player_id = None

    def step(self, actions=[], player=None):
        player_ids = cycle([self.player_id]) if player is None else cycle(player)

        cmds = Actions()
        cmds.actions.extend([
            Action(content=json.dumps(a), playerID=pid) for (a, pid) in zip(actions, player_ids) if a is not None
        ])
        res = self.stub.Step(cmds)
        self.current_state = GameState(json.loads(res.content), self)
        return self.current_state

    def reset(self, config=None):
        if config is not None:
            self.player_id = config.playerID or 1

        req = ResetRequest(scenario=config)
        res = self.stub.Reset(req)
        self.current_state = GameState(json.loads(res.content), self)
        return self.current_state

    def get_template(self, name):
        return self.get_templates([name])[0]

    def get_templates(self, names):
        req = GetTemplateRequest(names=names)
        res = self.stub.GetTemplates(req)
        return [ (t.name, EntityTemplate(t.content)) for t in res.templates ]

    def update_templates(self, types=[]):
        all_types = list(set([unit.type() for unit in self.current_state.units()]))
        all_types += types
        template_pairs = self.get_templates(all_types)

        self.cache = {}
        for (name, tpl) in template_pairs:
            self.cache[name] = tpl

        return template_pairs

class GameState():
    def __init__(self, data, game):
        self.data = data
        self.game = game
        self.mapSize = self.data['mapSize']

    def units(self, owner=None, type=None):
        filter_fn = lambda e: (owner is None or e['owner'] == owner) and \
                (type is None or type in e['template'])
        return [ Entity(e, self.game) for e in self.data['entities'].values() if filter_fn(e) ]

    def unit(self, id):
        id = str(id)
        return Entity(self.data['entities'][id], self.game) if id in self.data['entities'] else None

    def center(self, units=None):
        if units is None:
            units = self.units(owner=1)

        positions = [ unit.position() for unit in units ]
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
            dist = self.dist(unit.position(), position)
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

class Entity():

    def __init__(self, data, game):
        self.data = data
        self.game = game
        self.template = self.game.cache.get(self.type(), None)

    def type(self):
        return self.data['template']

    def id(self):
        return self.data['id']

    def owner(self):
        return self.data['owner']

    def max_health(self):
        template = self.get_template()
        return float(template.get('Health/Max'))

    def health(self, ratio=False):
        if ratio:
            return self.data['hitpoints']/self.max_health()

        return self.data['hitpoints']

    def position(self):
        return self.data['position']

    def get_template(self):
        if self.template is None:
            self.game.update_templates([self.type()])
            self.template = self.game.cache[self.type()]

        return self.template

# Class for reading/writing to the template. Used primarily for parsing the template...
class EntityTemplate():
    def __init__(self, xml):
        self.data = ElementTree.fromstring(f'<Entity>{xml}</Entity>')

    def get(self, path):
        node = self.data.find(path)
        return node.text if node is not None else None

    def set(self, path, value):
        node = self.data.find(path)
        if node:
            node.text = str(value)

        return node is not None

    def __str__(self):
        return ElementTree.tostring(self.data).decode('utf-8')

