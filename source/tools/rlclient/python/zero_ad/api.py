import urllib
from urllib import request
import json

class RLAPI():
    def __init__(self, url):
        self.url = url

    def post(self, route, data):
        response = request.urlopen(url=f'{self.url}/{route}', data=bytes(data, 'utf8'))
        return response.read()

    def step(self, commands):
        post_data = '\n'.join((f'{player};{json.dumps(action)}' for (player, action) in commands))
        return self.post('step', post_data)

    def reset(self, scenario_config, player_id, save_replay):
        path = 'reset?'
        if save_replay:
            path += 'saveReplay=1&'
        if player_id:
            path += f'playerID={player_id}&'

        return self.post(path, scenario_config)

    def get_templates(self, names):
        post_data = '\n'.join(names)
        response = self.post('templates', post_data)
        return zip(names, response.decode().split('\n'))
