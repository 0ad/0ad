#!/usr/bin/env python3
import argparse
import os
import re
import time
import xml.etree.ElementTree
from logging import getLogger, StreamHandler, INFO, Formatter

class Actor:
    def __init__(self, mod_name, vfs_path):
        self.mod_name = mod_name
        self.vfs_path = vfs_path
        self.name = os.path.basename(vfs_path)
        self.textures = []
        self.material = ''
        self.logger = getLogger(__name__)

    def read(self, physical_path):
        try:
            tree = xml.etree.ElementTree.parse(physical_path)
        except xml.etree.ElementTree.ParseError as err:
            self.logger.error('"%s": %s\n' % (physical_path, err.msg))
            return False
        root = tree.getroot()
        # Special case: particles don't need a diffuse texture.
        if len(root.findall('.//particles')) > 0:
            self.textures.append("baseTex")

        for element in root.findall('.//material'):
            self.material = element.text
        for element in root.findall('.//texture'):
            self.textures.append(element.get('name'))
        for element in root.findall('.//variant'):
            file = element.get('file')
            if file:
                self.read_variant(physical_path, os.path.join('art', 'variants', file))
        return True

    def read_variant(self, actor_physical_path, relative_path):
        physical_path = actor_physical_path.replace(self.vfs_path, relative_path)
        try:
            tree = xml.etree.ElementTree.parse(physical_path)
        except xml.etree.ElementTree.ParseError as err:
            self.logger.error('"%s": %s\n' % (physical_path, err.msg))
            return False

        root = tree.getroot()
        file = root.get('file')
        if file:
            self.read_variant(actor_physical_path, os.path.join('art', 'variants', file))

        for element in root.findall('.//texture'):
            self.textures.append(element.get('name'))


class Material:
    def __init__(self, mod_name, vfs_path):
        self.mod_name = mod_name
        self.vfs_path = vfs_path
        self.name = os.path.basename(vfs_path)
        self.required_textures = []

    def read(self, physical_path):
        try:
            root = xml.etree.ElementTree.parse(physical_path).getroot()
        except xml.etree.ElementTree.ParseError as err:
            self.logger.error('"%s": %s\n' % (physical_path, err.msg))
            return False
        for element in root.findall('.//required_texture'):
            texture_name = element.get('name')
            self.required_textures.append(texture_name)
        return True


class Validator:
    def __init__(self, vfs_root, mods=None):
        if mods is None:
            mods = ['mod', 'public']

        self.vfs_root = vfs_root
        self.mods = mods
        self.materials = {}
        self.invalid_materials = {}
        self.actors = []
        self.__init_logger

    @property
    def __init_logger(self):
        logger = getLogger(__name__)
        logger.setLevel(INFO)
        ch = StreamHandler()
        ch.setLevel(INFO)
        ch.setFormatter(Formatter('%(levelname)s - %(message)s'))
        logger.addHandler(ch)
        self.logger = logger

    def get_physical_path(self, mod_name, vfs_path):
        return os.path.realpath(os.path.join(self.vfs_root, mod_name, vfs_path))

    def find_mod_files(self, mod_name, vfs_path, pattern):
        physical_path = self.get_physical_path(mod_name, vfs_path)
        result = []
        if not os.path.isdir(physical_path):
            return result
        for file_name in os.listdir(physical_path):
            if file_name == '.git' or file_name == '.svn':
                continue
            vfs_file_path = os.path.join(vfs_path, file_name)
            physical_file_path = os.path.join(physical_path, file_name)
            if os.path.isdir(physical_file_path):
                result += self.find_mod_files(mod_name, vfs_file_path, pattern)
            elif os.path.isfile(physical_file_path) and pattern.match(file_name):
                result.append({
                    'mod_name': mod_name,
                    'vfs_path': vfs_file_path
                })
        return result

    def find_all_mods_files(self, vfs_path, pattern):
        result = []
        for mod_name in reversed(self.mods):
            result += self.find_mod_files(mod_name, vfs_path, pattern)
        return result

    def find_materials(self, vfs_path):
        material_files = self.find_all_mods_files(vfs_path, re.compile(r'.*\.xml'))
        for material_file in material_files:
            material_name = os.path.basename(material_file['vfs_path'])
            if material_name in self.materials:
                continue
            material = Material(material_file['mod_name'], material_file['vfs_path'])
            if material.read(self.get_physical_path(material_file['mod_name'], material_file['vfs_path'])):
                self.materials[material_name] = material
            else:
                self.invalid_materials[material_name] = material

    def find_actors(self, vfs_path):
        actor_files = self.find_all_mods_files(vfs_path, re.compile(r'.*\.xml'))
        for actor_file in actor_files:
            actor = Actor(actor_file['mod_name'], actor_file['vfs_path'])
            if actor.read(self.get_physical_path(actor_file['mod_name'], actor_file['vfs_path'])):
                self.actors.append(actor)

    def run(self):
        start_time = time.time()

        self.logger.info('Collecting list of files to check\n')

        self.find_materials(os.path.join('art', 'materials'))
        self.find_actors(os.path.join('art', 'actors'))

        for actor in self.actors:
            if not actor.material:
                continue
            if actor.material not in self.materials and actor.material not in self.invalid_materials:
                self.logger.error('"%s": unknown material "%s"' % (
                    self.get_mod_path(actor.mod_name, actor.vfs_path),
                    actor.material
                ))
            if actor.material not in self.materials:
                continue
            material = self.materials[actor.material]

            missing_textures = ', '.join(set([required_texture for required_texture in material.required_textures if required_texture not in actor.textures]))
            if len(missing_textures) > 0:
                self.logger.error('"%s": actor does not contain required texture(s) "%s" from "%s"' % (
                    self.get_mod_path(actor.mod_name, actor.vfs_path),
                    missing_textures,
                    material.name
                ))

            extra_textures = ', '.join(set([extra_texture for extra_texture in actor.textures if extra_texture not in material.required_textures]))
            if len(extra_textures) > 0:
                self.logger.warning('"%s": actor contains unnecessary texture(s) "%s" from "%s"' % (
                    self.get_mod_path(actor.mod_name, actor.vfs_path),
                    extra_textures,
                    material.name
                ))

        finish_time = time.time()
        self.logger.info('Total execution time: %.3f seconds.\n' % (finish_time - start_time))

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.realpath(__file__))
    default_root = os.path.join(script_dir, '..', '..', '..', 'binaries', 'data', 'mods')
    parser = argparse.ArgumentParser(description='Actors/materials validator.')
    parser.add_argument('-r', '--root', action='store', dest='root', default=default_root)
    parser.add_argument('-m', '--mods', action='store', dest='mods', default='mod,public')
    args = parser.parse_args()
    validator = Validator(args.root, args.mods.split(','))
    validator.run()
