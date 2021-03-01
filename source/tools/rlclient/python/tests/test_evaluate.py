import zero_ad
import json
import math
from os import path

game = zero_ad.ZeroAD('http://localhost:6000')
scriptdir = path.dirname(path.realpath(__file__))
with open(path.join(scriptdir, '..', 'samples', 'arcadia.json'), 'r') as f:
    config = f.read()

with open(path.join(scriptdir, 'fastactions.js'), 'r') as f:
    fastactions = f.read()

def test_return_object():
    state = game.reset(config)
    result = game.evaluate('({"hello": "world"})')
    assert type(result) is dict
    assert result['hello'] == 'world'

def test_return_null():
    result = game.evaluate('null')
    assert result == None

def test_return_string():
    state = game.reset(config)
    result = game.evaluate('"cat"')
    assert result == 'cat'

def test_fastactions():
    state = game.reset(config)
    game.evaluate(fastactions)
    female_citizens = state.units(owner=1, type='female_citizen')
    house_tpl = 'structures/spart/house'
    house_count = len(state.units(owner=1, type=house_tpl))
    x = 680
    z = 640
    build_house = zero_ad.actions.construct(female_citizens, house_tpl, x, z, autocontinue=True)
    # Check that they start building the house
    state = game.step([build_house])
    step_count = 0
    new_house = lambda _=None: state.units(owner=1, type=house_tpl)[0]
    initial_health = new_house().health(ratio=True)
    while new_house().health(ratio=True) == initial_health:
        state = game.step()

    assert new_house().health(ratio=True) >= 1.0
