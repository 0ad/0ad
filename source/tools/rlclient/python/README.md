# 0 AD Python Client
This directory contains `zero_ad`, a python client for 0 AD which enables users to control the environment headlessly.

## Installation
`zero_ad` can be installed with `pip` by running the following from the current directory:
```
pip install .
```

Development dependencies can be installed with `pip install -r requirements-dev.txt`. Tests are using pytest and can be run with `python -m pytest`.

## Basic Usage
If there is not a running instance of 0 AD, first start 0 AD with the RL interface enabled:
```
pyrogenesis --rl-interface=127.0.0.1:6000
```

Next, the python client can be connected with:
```
import zero_ad
from zero_ad import ZeroAD

game = ZeroAD('http://localhost:6000')
```

A map can be loaded with:

```
with open('./samples/arcadia.json', 'r') as f:
    arcadia_config = f.read()

state = game.reset(arcadia_config)
```

where `./samples/arcadia.json` is the path to a game configuration JSON (included in the first line of the commands.txt file in a game replay directory) and `state` contains the initial game state for the given map. The game engine can be stepped (optionally applying actions at each step) with:

```
state = game.step()
```

For example, enemy units could be attacked with:

```
my_units = state.units(owner=1)
enemy_units = state.units(owner=2)
actions = [zero_ad.actions.attack(my_units, enemy_units[0])]
state = game.step(actions)
```

For a more thorough example, check out samples/simple-example.py!
