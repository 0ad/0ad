def debug_print(message):
    return {
        'type': 'debug-print',
        'message': message
    }

def chat(message):
    return {
        'type': 'chat',
        'message': message
    }

def diplomacy(player, status='ally'):
    return {
        'type': 'diplomacy',
        'player': player,
        'to': status,  # ally, neutral or enemy
    }

def tribute(player, amounts):
    return {
        'type': 'diplomacy',
        'player': player,
        'amounts': amounts  # TODO: What should this be??
    }

def walk(units, x, z, queued=False):
    ids = [ unit.id() for unit in units ]
    return {
        'type': 'walk',
        'entities': ids,
        'x': x,
        'z': z,
        'queued': queued
    }

def attack(units, target, queued=False, allow_capture=True):
    unit_ids = [ unit.id() for unit in units ]
    return {
        'type': 'attack',
        'entities': unit_ids,
        'target': target.id(),
        'allowCapture': allow_capture,
        'queued': queued
    }

