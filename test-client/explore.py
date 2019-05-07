import RLAPI_pb2_grpc
import RLAPI_pb2
import grpc
import time
import json
import sys
from random import randint
import math

channel = grpc.insecure_channel('localhost:50051')
stub = RLAPI_pb2_grpc.RLAPIStub(channel)

print('Connecting...')
req = RLAPI_pb2.ConnectRequest()
obs = stub.Connect(req)
print('Connected', file=sys.stderr)

# Step the game engine and get a unit from the state
cmds = RLAPI_pb2.Actions()
obs = stub.Step(cmds)
state = json.loads(obs.content)
map_size = state['mapSize']
my_entities = [ str(ent['id']) for ent in state['entities'].values() if ent['owner'] == 1 and ent['template'].startswith('unit') ]

goal_locs = {}
for ent in my_entities:
    goal_locs[ent] = state['entities'][ent]['position']

def move_to_new_loc(entities):
    cmds = RLAPI_pb2.Actions()
    for ent in entities:
        x, z = [ randint(1, map_size) for _ in range(2) ]
        print('moving {} to {}, {}'.format(ent, x, z), file=sys.stderr)
        goal_locs[ent] = [x, z]

    unit_cmds = [ {"type": "walk", "entities": [int(ent)], "x": goal_locs[ent][0], "z": goal_locs[ent][1], "queued": False} for ent in entities ]
    cmds.actions.extend([
        RLAPI_pb2.Action(content=json.dumps(cmd)) for cmd in unit_cmds
    ])

    return json.loads(stub.Step(cmds).content)

def dist(p1, p2):
    return math.sqrt(math.pow(p2[0]-p1[0], 2) + math.pow(p2[1]-p1[1], 2))

while True:
    ent_positions = [ [e, state['entities'][e]['position']] for e in my_entities if e in state['entities'] ]
    ent_dists = [ [e, dist(pos, goal_locs.get(e, [0, 0]))] for (e, pos) in ent_positions ]
    idle_ents = [ e for (e, dist) in ent_dists if dist < 20 ]
    state = move_to_new_loc(idle_ents)
    #time.sleep(1)
