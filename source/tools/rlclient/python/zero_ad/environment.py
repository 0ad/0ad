from .api import RLAPI
import json
import math
from xml.etree import ElementTree
from itertools import cycle

class ZeroAD():
    def __init__(self, uri='http://localhost:6000'):
        self.api = RLAPI(uri)
        self.current_state = None
        self.cache = {}
        self.player_id = 1

    def step(self, actions=[], player=None):
        player_ids = cycle([self.player_id]) if player is None else cycle(player)

        cmds = zip(player_ids, actions)
        cmds = ((player, action) for (player, action) in cmds if action is not None)
        state_json = self.api.step(cmds)
        self.current_state = GameState(json.loads(state_json), self)
        return self.current_state

    def reset(self, config='', save_replay=False, player_id=1):
        state_json = self.api.reset(config, player_id, save_replay)
        self.current_state = GameState(json.loads(state_json), self)
        return self.current_state

    def get_template(self, name):
        return self.get_templates([name])[0]

    def get_templates(self, names):
        templates = self.api.get_templates(names)
        return [ (name, EntityTemplate(content)) for (name, content) in templates ]

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
