# This script provides an overview of the zero_ad wrapper for 0 AD
from os import path
import zero_ad

# First, we will define some helper functions we will use later.
import math
def dist (p1, p2):
    return math.sqrt(sum((math.pow(x2 - x1, 2) for (x1, x2) in zip(p1, p2))))

def center(units):
    sum_position = map(sum, zip(*map(lambda u: u.position(), units)))
    return [x/len(units) for x in sum_position]

def closest(units, position):
    dists = (dist(unit.position(), position) for unit in units)
    index = 0
    min_dist = next(dists)
    for (i, d) in enumerate(dists):
        if d < min_dist:
            index = i
            min_dist = d

    return units[index]

# Connect to a 0 AD game server listening at localhost:6000
game = zero_ad.ZeroAD('http://localhost:6000')

# Load the Arcadia map
samples_dir = path.dirname(path.realpath(__file__))
scenario_config_path = path.join(samples_dir, 'arcadia.json')
with open(scenario_config_path, 'r') as f:
    arcadia_config = f.read()

state = game.reset(arcadia_config)

# The game is paused and will only progress upon calling "step"
state = game.step()

# Units can be queried from the game state
citizen_soldiers = state.units(owner=1, type='infantry')
# (including gaia units like trees or other resources)
nearby_tree = closest(state.units(owner=0, type='tree'), center(citizen_soldiers))

# Action commands can be created using zero_ad.actions
collect_wood = zero_ad.actions.gather(citizen_soldiers, nearby_tree)

female_citizens = state.units(owner=1, type='female_citizen')
house_tpl = 'structures/spart_house'
x = 680
z = 640
build_house = zero_ad.actions.construct(female_citizens, house_tpl, x, z, autocontinue=True)

# These commands can then be applied to the game in a `step` command
state = game.step([collect_wood, build_house])

# We can also fetch units by id using the `unit` function on the game state
female_id = female_citizens[0].id()
female_citizen = state.unit(female_id)

# A variety of unit information can be queried from the unit:
print('female citizen\'s max health is', female_citizen.max_health())

# Raw data for units and game states are available via the data attribute
print(female_citizen.data)

# Units can be built using the "train action"
civic_center = state.units(owner=1, type="civil_centre")[0]
spearman_type = 'units/spart_infantry_spearman_b'
train_spearmen = zero_ad.actions.train([civic_center], spearman_type)

state = game.step([train_spearmen])

# Let's step the engine until the house has been built
is_unit_busy = lambda state, unit_id: len(state.unit(unit_id).data['unitAIOrderData']) > 0
while is_unit_busy(state, female_id):
    state = game.step()

# The units for the other army can also be controlled
enemy_units = state.units(owner=2)
walk = zero_ad.actions.walk(enemy_units, *civic_center.position())
game.step([walk], player=[2])

# Step the game engine a bit to give them some time to walk
for _ in range(150):
    state = game.step()

# Let's attack with our entire military
state = game.step([zero_ad.actions.chat('An attack is coming!')])

while len(state.units(owner=2, type='unit')) > 0:
    attack_units = [ unit for unit in state.units(owner=1, type='unit') if 'female' not in unit.type() ]
    target = closest(state.units(owner=2, type='unit'), center(attack_units))
    state = game.step([zero_ad.actions.attack(attack_units, target)])

    while state.unit(target.id()):
        state = game.step()

game.step([zero_ad.actions.chat('The enemies have been vanquished. Our home is safe again.')])
