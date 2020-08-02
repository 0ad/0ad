def construct(units, template, x, z, angle=0, autorepair=True, autocontinue=True, queued=False):
    unit_ids = [ unit.id() for unit in units ]
    return {
        'type': 'construct',
        'entities': unit_ids,
        'template': template,
        'x': x,
        'z': z,
        'angle': angle,
        'autorepair': autorepair,
        'autocontinue': autocontinue,
        'queued': queued,
    }

def gather(units, target, queued=False):
    unit_ids = [ unit.id() for unit in units ]
    return {
        'type': 'gather',
        'entities': unit_ids,
        'target': target.id(),
        'queued': queued,
    }

def train(entities, unit_type, count=1):
    entity_ids = [ unit.id() for unit in entities ]
    return {
        'type': 'train',
        'entities': entity_ids,
        'template': unit_type,
        'count': count,
    }

def debug_print(message):
    return {
        'type': 'debug-print',
        'message': message
    }

def chat(message):
    return {
        'type': 'aichat',
        'message': message
    }

def reveal_map():
    return {
        'type': 'reveal-map',
        'enable': True
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
