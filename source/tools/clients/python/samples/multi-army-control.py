# This script is a simple example of how to control multiple armies. This uses the default scenario (Arcadia)
# and sends both armies to the other's home base
import zero_ad
import time

game = zero_ad.ZeroAD('localhost:50051')
state = game.reset()

# Move enemy units to home base and ours to the enemy base
units = state.units(owner=1)
home_base = state.units(owner=1)[0].position()
enemies = state.units(owner=2)
enemy_base = state.units(owner=2)[0].position()
print(f'moving all ({len(enemies)}) enemies to home base at ', *home_base)
print(f'moving all ({len(units)}) units to enemy base at ', *enemy_base)
actions = [zero_ad.actions.walk(enemies, *home_base), zero_ad.actions.walk(units, *enemy_base)]
new_state = game.step(actions, player=[2, 1])

while 1:
    state = game.step()
    time.sleep(1)
