import zero_ad
import json
import math
from os import path

game = zero_ad.ZeroAD('http://localhost:6000')
scriptdir = path.dirname(path.realpath(__file__))
with open(path.join(scriptdir, '..', 'samples', 'arcadia.json'), 'r') as f:
    config = f.read()

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

def test_construct():
    state = game.reset(config)
    female_citizens = state.units(owner=1, type='female_citizen')
    house_tpl = 'structures/spart_house'
    house_count = len(state.units(owner=1, type=house_tpl))
    x = 680
    z = 640
    build_house = zero_ad.actions.construct(female_citizens, house_tpl, x, z, autocontinue=True)
    # Check that they start building the house
    state = game.step([build_house])
    while len(state.units(owner=1, type=house_tpl)) == house_count:
        state = game.step()

def test_gather():
    state = game.reset(config)
    female_citizen = state.units(owner=1, type='female_citizen')[0]
    trees = state.units(owner=0, type='tree')
    nearby_tree = closest(state.units(owner=0, type='tree'), female_citizen.position())

    collect_wood = zero_ad.actions.gather([female_citizen], nearby_tree)
    state = game.step([collect_wood])
    while len(state.unit(female_citizen.id()).data['resourceCarrying']) == 0:
        state = game.step()

def test_train():
    state = game.reset(config)
    civic_centers = state.units(owner=1, type="civil_centre")
    spearman_type = 'units/spart_infantry_spearman_b'
    spearman_count = len(state.units(owner=1, type=spearman_type))
    train_spearmen = zero_ad.actions.train(civic_centers, spearman_type)

    state = game.step([train_spearmen])
    while len(state.units(owner=1, type=spearman_type)) == spearman_count:
        state = game.step()

def test_walk():
    state = game.reset(config)
    female_citizens = state.units(owner=1, type='female_citizen')
    x = 680
    z = 640
    initial_distance = dist(center(female_citizens), [x, z])

    walk = zero_ad.actions.walk(female_citizens, x, z)
    state = game.step([walk])
    distance = initial_distance
    while distance >= initial_distance:
        state = game.step()
        female_citizens = state.units(owner=1, type='female_citizen')
        distance = dist(center(female_citizens), [x, z])

def test_attack():
    state = game.reset(config)
    units = state.units(owner=1, type='cavalry')
    target = state.units(owner=2, type='female_citizen')[0]
    initial_health = target.health()

    state = game.step([zero_ad.actions.reveal_map()])

    attack = zero_ad.actions.attack(units, target)
    state = game.step([attack])
    while state.unit(target.id()).health() >= initial_health:
        state = game.step()

def test_debug_print():
    state = game.reset(config)
    debug_print = zero_ad.actions.debug_print('hello world!!')
    state = game.step([debug_print])

def test_chat():
    state = game.reset(config)
    chat = zero_ad.actions.chat('hello world!!')
    state = game.step([chat])
