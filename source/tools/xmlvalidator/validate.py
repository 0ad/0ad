#!/usr/bin/env python3
from argparse import ArgumentParser
from pathlib import Path
from os.path import sep, join, realpath, exists, basename, dirname
from json import load, loads
from re import split, match
from logging import getLogger, StreamHandler, INFO, Formatter
import lxml.etree

class VFS_File:
    def __init__(self, mod_name, vfs_path):
        self.mod_name = mod_name
        self.vfs_path = vfs_path

class RelaxNGValidator:
    def __init__(self, vfs_root, mods=None, verbose=False):
        self.mods = mods if mods is not None else []
        self.vfs_root = Path(vfs_root)
        self.__init_logger
        self.verbose = verbose

    @property
    def __init_logger(self):
        logger = getLogger(__name__)
        logger.setLevel(INFO)
        # create a console handler, seems nicer to Windows and for future uses
        ch = StreamHandler()
        ch.setLevel(INFO)
        ch.setFormatter(Formatter('%(levelname)s - %(message)s'))
        # ch.setFormatter(Formatter('%(message)s')) # same output as perl
        logger.addHandler(ch)
        self.logger = logger

    def run (self):
        self.validate_actors()
        self.validate_variants()
        self.validate_guis()
        self.validate_maps()
        self.validate_materials()
        self.validate_particles()
        self.validate_simulation()
        self.validate_soundgroups()
        self.validate_terrains()
        self.validate_textures()

    def main(self):
        """ Program entry point, parses command line arguments and launches the validation """
        # ordered uniq mods (dict maintains ordered keys from python 3.6)
        self.logger.info(f"Checking {'|'.join(self.mods)}'s integrity.")
        self.logger.info(f"The following mods will be loaded: {'|'.join(self.mods)}.")
        self.run()

    def find_files(self, vfs_root, mods, vfs_path, *ext_list):
        """
        returns a list of 2-size tuple with:
            - Path relative to the mod base
            - full Path
        """
        full_exts = ['.' + ext for ext in ext_list]

        def find_recursive(dp, base):
            """(relative Path, full Path) generator"""
            if dp.is_dir():
                if dp.name != '.svn' and dp.name != '.git' and not dp.name.endswith('~'):
                    for fp in dp.iterdir():
                        yield from find_recursive(fp, base)
            elif dp.suffix in full_exts:
                relative_file_path = dp.relative_to(base)
                yield (relative_file_path, dp.resolve())
        return [(rp, fp) for mod in mods for (rp, fp) in find_recursive(vfs_root / mod / vfs_path, vfs_root / mod)]

    def validate_actors(self):
        self.logger.info('Validating actors...')
        files = self.find_files(self.vfs_root, self.mods, 'art/actors/', 'xml')
        self.validate_files('actors', files, 'art/actors/actor.rng')

    def validate_variants(self):
        self.logger.info("Validating variants...")
        files = self.find_files(self.vfs_root, self.mods, 'art/variants/', 'xml')
        self.validate_files('variant', files, 'art/variants/variant.rng')

    def validate_guis(self):
        self.logger.info("Validating gui files...")
        pages = [file for file in self.find_files(self.vfs_root, self.mods, 'gui/', 'xml') if match(r".*[\\\/]page(_[^.\/\\]+)?\.xml$", str(file[0]))]
        self.validate_files('gui page', pages, 'gui/gui_page.rng')
        xmls = [file for file in self.find_files(self.vfs_root, self.mods, 'gui/', 'xml') if not match(r".*[\\\/]page(_[^.\/\\]+)?\.xml$", str(file[0]))]
        self.validate_files('gui xml', xmls, 'gui/gui.rng')

    def validate_maps(self):
        self.logger.info("Validating maps...")
        files = self.find_files(self.vfs_root, self.mods, 'maps/scenarios/', 'xml')
        self.validate_files('map', files, 'maps/scenario.rng')
        files = self.find_files(self.vfs_root, self.mods, 'maps/skirmishes/', 'xml')
        self.validate_files('map', files, 'maps/scenario.rng')

    def validate_materials(self):
        self.logger.info("Validating materials...")
        files = self.find_files(self.vfs_root, self.mods, 'art/materials/', 'xml')
        self.validate_files('material', files, 'art/materials/material.rng')

    def validate_particles(self):
        self.logger.info("Validating particles...")
        files = self.find_files(self.vfs_root, self.mods, 'art/particles/', 'xml')
        self.validate_files('particle', files, 'art/particles/particle.rng')

    def validate_simulation(self):
        self.logger.info("Validating simulation...")
        file = self.find_files(self.vfs_root, self.mods, 'simulation/data/pathfinder', 'xml')
        self.validate_files('pathfinder', file, 'simulation/data/pathfinder.rng')
        file = self.find_files(self.vfs_root, self.mods, 'simulation/data/territorymanager', 'xml')
        self.validate_files('territory manager', file, 'simulation/data/territorymanager.rng')

    def validate_soundgroups(self):
        self.logger.info("Validating soundgroups...")
        files = self.find_files(self.vfs_root, self.mods, 'audio/', 'xml')
        self.validate_files('sound group', files, 'audio/sound_group.rng')

    def validate_terrains(self):
        self.logger.info("Validating terrains...")
        terrains = [file for file in self.find_files(self.vfs_root, self.mods, 'art/terrains/', 'xml') if 'terrains.xml' in str(file[0])]
        self.validate_files('terrain', terrains, 'art/terrains/terrain.rng')
        terrains_textures = [file for file in self.find_files(self.vfs_root, self.mods, 'art/terrains/', 'xml') if 'terrains.xml' not in str(file[0])]
        self.validate_files('terrain texture', terrains_textures, 'art/terrains/terrain_texture.rng')

    def validate_textures(self):
        self.logger.info("Validating textures...")
        files = [file for file in self.find_files(self.vfs_root, self.mods, 'art/textures/', 'xml') if 'textures.xml' in str(file[0])]
        self.validate_files('texture', files, 'art/textures/texture.rng')

    def get_physical_path(self, mod_name, vfs_path):
        return realpath(join(self.vfs_root, mod_name, vfs_path))

    def get_relaxng_file(self, schemapath):
            """We look for the highest priority mod relax NG file"""
            for mod in self.mods:
                relax_ng_path = self.get_physical_path(mod, schemapath)
                if exists(relax_ng_path):
                    return relax_ng_path

            return ""

    def validate_files(self, name, files, schemapath):
        relax_ng_path = self.get_relaxng_file(schemapath)
        if relax_ng_path == "":
            self.logger.warning(f"Could not find {schemapath}")
            return

        data = lxml.etree.parse(relax_ng_path)
        relaxng = lxml.etree.RelaxNG(data)
        error_count = 0
        for file in sorted(files):
            try:
                doc = lxml.etree.parse(str(file[1]))
                relaxng.assertValid(doc)
            except Exception as e:
                error_count = error_count + 1
                self.logger.error(f"{file[1]}: " + str(e))

        if self.verbose:
            self.logger.info(f"{error_count} {name} validation errors")
        elif error_count > 0:
            self.logger.error(f"{error_count} {name} validation errors")


def get_mod_dependencies(vfs_root, *mods):
    modjsondeps = []
    for mod in mods:
        mod_json_path = Path(vfs_root) / mod / 'mod.json'
        if not exists(mod_json_path):
            continue

        with open(mod_json_path, encoding='utf-8') as f:
            modjson = load(f)
        # 0ad's folder isn't named like the mod.
        modjsondeps.extend(['public' if '0ad' in dep else dep for dep in modjson.get('dependencies', [])])
    return modjsondeps

if __name__ == '__main__':
    script_dir = dirname(realpath(__file__))
    default_root = join(script_dir, '..', '..', '..', 'binaries', 'data', 'mods')
    ap = ArgumentParser(description="Validates XML files againt their Relax NG schemas")
    ap.add_argument('-r', '--root', action='store', dest='root', default=default_root)
    ap.add_argument('-v', '--verbose', action='store_true', default=True,
                help="Log validation errors.")
    ap.add_argument('-m', '--mods', metavar="MOD", dest='mods', nargs='+', default=['public'],
                    help="specify which mods to check. Default to public and mod.")
    args = ap.parse_args()
    mods = list(dict.fromkeys([*args.mods, *get_mod_dependencies(args.root, *args.mods), 'mod']).keys())
    relax_ng_validator = RelaxNGValidator(args.root, mods=mods, verbose=args.verbose)
    relax_ng_validator.main()