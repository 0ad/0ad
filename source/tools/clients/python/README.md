# 0 AD Python Client
This directory contains `zero_ad`, a python client for 0 AD which enables users to control the environment headlessly.

## Basic Usage
Assuming there is an instance of 0 AD running with the `--rpc-server` flag at `<URI>`, this wrapper can connect to it via:

```
import zero_ad
from zero_ad import ZeroAD

game = ZeroAD(<URI>)
```

A map can be loaded with:

```
config = zero_ad.ScenarioConfig(type='scenario', name='Arcadia')
state = game.reset(config)
```

where `state` contains the initial game state for the given map. The game engine can be stepped (optionally applying actions at each step) with:

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
