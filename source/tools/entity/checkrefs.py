#!/usr/bin/env python3
from argparse import ArgumentParser
from io import BytesIO
from json import load, loads
from pathlib import Path
from re import split, match
from struct import unpack, calcsize
from os.path import sep, exists, basename
from xml.etree import ElementTree
import sys
from scriptlib import SimulTemplateEntity, find_files
from logging import WARNING, getLogger, StreamHandler, INFO, Formatter, Filter

class SingleLevelFilter(Filter):
    def __init__(self, passlevel, reject):
        self.passlevel = passlevel
        self.reject = reject

    def filter(self, record):
        if self.reject:
            return (record.levelno != self.passlevel)
        else:
            return (record.levelno == self.passlevel)

class CheckRefs:
    def __init__(self):
        # list of relative root file:str
        self.files = []
        # list of relative file:str
        self.roots = []
        # list of tuple (parent_file:str, dep_file:str)
        self.deps = []
        self.vfs_root = Path(__file__).resolve().parents[3] / 'binaries' / 'data' / 'mods'
        self.supportedTextureFormats = ('dds', 'png')
        self.supportedMeshesFormats = ('pmd', 'dae')
        self.supportedAnimationFormats = ('psa', 'dae')
        self.supportedAudioFormats = ('ogg')
        self.mods = []
        self.__init_logger

    @property
    def __init_logger(self):
        logger = getLogger(__name__)
        logger.setLevel(INFO)
        # create a console handler, seems nicer to Windows and for future uses
        ch = StreamHandler(sys.stdout)
        ch.setLevel(INFO)
        ch.setFormatter(Formatter('%(levelname)s - %(message)s'))
        f1 = SingleLevelFilter(INFO, False)
        ch.addFilter(f1)
        logger.addHandler(ch)
        errorch = StreamHandler(sys.stderr)
        errorch.setLevel(WARNING)
        errorch.setFormatter(Formatter('%(levelname)s - %(message)s'))
        logger.addHandler(errorch)
        self.logger = logger

    def main(self):
        ap = ArgumentParser(description="Checks the game files for missing dependencies, unused files,"
                            " and for file integrity.")
        ap.add_argument('-u', '--check-unused', action='store_true',
                        help="check for all the unused files in the given mods and their dependencies."
                        " Implies --check-map-xml. Currently yields a lot of false positives.")
        ap.add_argument('-x', '--check-map-xml', action='store_true',
                        help="check maps for missing actor and templates.")
        ap.add_argument('-a', '--validate-actors', action='store_true',
                        help="run the validator.py script to check if the actors files have extra or missing textures."
                        " This currently only works for the public mod.")
        ap.add_argument('-t', '--validate-templates', action='store_true',
                        help="run the validator.py script to check if the xml files match their (.rng) grammar file.")
        ap.add_argument('-m', '--mods', metavar="MOD", dest='mods', nargs='+', default=['public'],
                        help="specify which mods to check. Default to public.")
        args = ap.parse_args()
        # force check_map_xml if check_unused is used to avoid false positives.
        args.check_map_xml |= args.check_unused
        # ordered uniq mods (dict maintains ordered keys from python 3.6)
        self.mods = list(dict.fromkeys([*args.mods, *self.get_mod_dependencies(*args.mods), 'mod']).keys())
        self.logger.info(f"Checking {'|'.join(args.mods)}'s integrity.")
        self.logger.info(f"The following mods will be loaded: {'|'.join(self.mods)}.")
        if args.check_map_xml:
            self.add_maps_xml()
        self.add_maps_pmp()
        self.add_entities()
        self.add_actors()
        self.add_variants()
        self.add_art()
        self.add_materials()
        self.add_particles()
        self.add_soundgroups()
        self.add_audio()
        self.add_gui_xml()
        self.add_gui_data()
        self.add_civs()
        self.add_rms()
        self.add_techs()
        self.add_terrains()
        self.add_auras()
        self.add_tips()
        self.check_deps()
        if args.check_unused:
            self.check_unused()
        if args.validate_templates:
            sys.path.append("../xmlvalidator/")
            from validate_grammar import RelaxNGValidator
            validate = RelaxNGValidator(self.vfs_root, self.mods)
            validate.run()
        if args.validate_actors:
            sys.path.append("../xmlvalidator/")
            from validator import Validator
            validator = Validator(self.vfs_root, self.mods)
            validator.run()

    def get_mod_dependencies(self, *mods):
        modjsondeps = []
        for mod in mods:
            mod_json_path = self.vfs_root / mod / 'mod.json'
            if not exists(mod_json_path):
                continue

            with open(mod_json_path, encoding='utf-8') as f:
                modjson = load(f)
            # 0ad's folder isn't named like the mod.
            modjsondeps.extend(['public' if '0ad' in dep else dep for dep in modjson.get('dependencies', [])])
        return modjsondeps

    def vfs_to_relative_to_mods(self, vfs_path):
        for dep in self.mods:
            fn = Path(dep) / vfs_path
            if (self.vfs_root / fn).exists():
                return fn
        return None

    def vfs_to_physical(self, vfs_path):
        fn = self.vfs_to_relative_to_mods(vfs_path)
        return self.vfs_root / fn

    def find_files(self, vfs_path, *ext_list):
        return find_files(self.vfs_root, self.mods, vfs_path, *ext_list)

    def add_maps_xml(self):
        self.logger.info("Loading maps XML...")
        mapfiles = self.find_files('maps/scenarios', 'xml')
        mapfiles.extend(self.find_files('maps/skirmishes', 'xml'))
        mapfiles.extend(self.find_files('maps/tutorials', 'xml'))
        actor_prefix = 'actor|'
        resource_prefix = 'resource|'
        for (fp, ffp) in sorted(mapfiles):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            et_map = ElementTree.parse(ffp).getroot()
            entities = et_map.find('Entities')
            used = {entity.find('Template').text.strip() for entity in entities.findall('Entity')} if entities is not None else {}
            for template in used:
                if template.startswith(actor_prefix):
                    self.deps.append((str(fp), f'art/actors/{template[len(actor_prefix):]}'))
                elif template.startswith(resource_prefix):
                    self.deps.append((str(fp), f'simulation/templates/{template[len(resource_prefix):]}.xml'))
                else:
                    self.deps.append((str(fp), f'simulation/templates/{template}.xml'))
            # Map previews
            settings = loads(et_map.find('ScriptSettings').text)
            if settings.get('Preview', None):
                self.deps.append((str(fp), f'art/textures/ui/session/icons/mappreview/{settings["Preview"]}'))

    def add_maps_pmp(self):
        self.logger.info("Loading maps PMP...")
        # Need to generate terrain texture filename=>relative path lookup first
        terrains = dict()
        for (fp, ffp) in self.find_files('art/terrains', 'xml'):
            name = fp.stem
            # ignore terrains.xml
            if name != 'terrains':
                if name in terrains:
                    self.logger.warning(f"Duplicate terrain name '{name}' (from '{terrains[name]}' and '{ffp}')")
                terrains[name] = str(fp)
        mapfiles = self.find_files('maps/scenarios', 'pmp')
        mapfiles.extend(self.find_files('maps/skirmishes', 'pmp'))
        for (fp, ffp) in sorted(mapfiles):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            with open(ffp, 'rb') as f:
                expected_header = b'PSMP'
                header = f.read(len(expected_header))
                if header != expected_header:
                    raise ValueError(f"Invalid PMP header {header} in '{ffp}'")
                int_fmt = '<L'  # little endian long int
                int_len = calcsize(int_fmt)
                version, = unpack(int_fmt, f.read(int_len))
                if version != 7:
                    raise ValueError(f"Invalid PMP version ({version}) in '{ffp}'")
                datasize, = unpack(int_fmt, f.read(int_len))
                mapsize, = unpack(int_fmt, f.read(int_len))
                f.seek(2 * (mapsize * 16 + 1) * (mapsize * 16 + 1), 1)  # skip heightmap
                numtexs, = unpack(int_fmt, f.read(int_len))
                for i in range(numtexs):
                    length, = unpack(int_fmt, f.read(int_len))
                    terrain_name = f.read(length).decode('ascii')  # suppose ascii encoding
                    self.deps.append((str(fp), terrains.get(terrain_name, f'art/terrains/(unknown)/{terrain_name}')))

    def add_entities(self):
        self.logger.info("Loading entities...")
        simul_templates_path = Path('simulation/templates')
        # TODO: We might want to get computed templates through the RL interface instead of computing the values ourselves.
        simul_template_entity = SimulTemplateEntity(self.vfs_root, self.logger)
        for (fp, _) in sorted(self.find_files(simul_templates_path, 'xml')):
            self.files.append(str(fp))
            entity = simul_template_entity.load_inherited(simul_templates_path, str(fp.relative_to(simul_templates_path)), self.mods)
            if entity.get('parent'):
                for parent in entity.get('parent').split('|'):
                    self.deps.append((str(fp), str(simul_templates_path / (parent + '.xml'))))
            if not str(fp).startswith('template_'):
                self.roots.append(str(fp))
                if entity and entity.find('VisualActor') is not None and entity.find('VisualActor').find('Actor') is not None:
                    if entity.find('Identity'):
                        phenotype_tag = entity.find('Identity').find('Phenotype')
                        phenotypes = split(r'\s', phenotype_tag.text if phenotype_tag is not None and phenotype_tag.text else 'default')
                        actor = entity.find('VisualActor').find('Actor')
                        if '{phenotype}' in actor.text:
                            for phenotype in phenotypes:
                                # See simulation2/components/CCmpVisualActor.cpp and Identity.js for explanation.
                                actor_path = actor.text.replace('{phenotype}', phenotype)
                                self.deps.append((str(fp), f'art/actors/{actor_path}'))
                        else:
                            actor_path = actor.text
                            self.deps.append((str(fp), f'art/actors/{actor_path}'))
                        foundation_actor = entity.find('VisualActor').find('FoundationActor')
                        if foundation_actor:
                            self.deps.append((str(fp), f'art/actors/{foundation_actor.text}'))
                if entity.find('Sound'):
                    phenotype_tag = entity.find('Identity').find('Phenotype')
                    phenotypes = split(r'\s', phenotype_tag.text if phenotype_tag is not None and phenotype_tag.text else 'default')
                    lang_tag = entity.find('Identity').find('Lang')
                    lang = lang_tag.text if lang_tag is not None and lang_tag.text else 'greek'
                    sound_groups = entity.find('Sound').find('SoundGroups')
                    for sound_group in sound_groups:
                        if sound_group.text and sound_group.text.strip():
                            if '{phenotype}' in sound_group.text:
                                for phenotype in phenotypes:
                                    # see simulation/components/Sound.js and Identity.js for explanation
                                    sound_path = sound_group.text.replace('{phenotype}', phenotype).replace('{lang}', lang)
                                    self.deps.append((str(fp), f'audio/{sound_path}'))
                            else:
                                sound_path = sound_group.text.replace('{lang}', lang)
                                self.deps.append((str(fp), f'audio/{sound_path}'))
                if entity.find('Identity') is not None:
                    icon = entity.find('Identity').find('Icon')
                    if icon is not None and icon.text:
                        if entity.find('Formation') is not None:
                            self.deps.append((str(fp), f'art/textures/ui/session/icons/{icon.text}'))
                        else:
                            self.deps.append((str(fp), f'art/textures/ui/session/portraits/{icon.text}'))
                if entity.find('Heal') is not None and entity.find('Heal').find('RangeOverlay') is not None:
                    range_overlay = entity.find('Heal').find('RangeOverlay')
                    for tag in ('LineTexture', 'LineTextureMask'):
                        elem = range_overlay.find(tag)
                        if elem is not None and elem.text:
                            self.deps.append((str(fp), f'art/textures/selection/{elem.text}'))
                if entity.find('Selectable') is not None and entity.find('Selectable').find('Overlay') is not None \
                        and entity.find('Selectable').find('Overlay').find('Texture') is not None:
                    texture = entity.find('Selectable').find('Overlay').find('Texture')
                    for tag in ('MainTexture', 'MainTextureMask'):
                        elem = texture.find(tag)
                        if elem is not None and elem.text:
                            self.deps.append((str(fp), f'art/textures/selection/{elem.text}'))
                if entity.find('Formation') is not None:
                    icon = entity.find('Formation').find('Icon')
                    if icon is not None and icon.text:
                        self.deps.append((str(fp), f'art/textures/ui/session/icons/{icon.text}'))

    def append_variant_dependencies(self, variant, fp):
        variant_file = variant.get('file')
        mesh = variant.find('mesh')
        particles = variant.find('particles')
        texture_files = [tex.get('file') for tex in variant.find('textures').findall('texture')] \
            if variant.find('textures') is not None else []
        prop_actors = [prop.get('actor') for prop in variant.find('props').findall('prop')] \
            if variant.find('props') is not None else []
        animation_files = [anim.get('file') for anim in variant.find('animations').findall('animation')] \
            if variant.find('animations') is not None else []
        if variant_file:
            self.deps.append((str(fp), f'art/variants/{variant_file}'))
        if mesh is not None and mesh.text:
            self.deps.append((str(fp), f'art/meshes/{mesh.text}'))
        if particles is not None and particles.get('file'):
            self.deps.append((str(fp), f'art/particles/{particles.get("file")}'))
        for texture_file in [x for x in texture_files if x]:
            self.deps.append((str(fp), f'art/textures/skins/{texture_file}'))
        for prop_actor in [x for x in prop_actors if x]:
            self.deps.append((str(fp), f'art/actors/{prop_actor}'))
        for animation_file in [x for x in animation_files if x]:
            self.deps.append((str(fp), f'art/animation/{animation_file}'))

    def append_actor_dependencies(self, actor, fp):
        for group in actor.findall('group'):
            for variant in group.findall('variant'):
                self.append_variant_dependencies(variant, fp)
        material = actor.find('material')
        if material is not None and material.text:
            self.deps.append((str(fp), f'art/materials/{material.text}'))

    def add_actors(self):
        self.logger.info("Loading actors...")
        for (fp, ffp) in sorted(self.find_files('art/actors', 'xml')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            root = ElementTree.parse(ffp).getroot()
            if root.tag == 'actor':
                self.append_actor_dependencies(root, fp)

            # model has lods
            elif root.tag == 'qualitylevels':
                qualitylevels = root
                for actor in qualitylevels.findall('actor'):
                    self.append_actor_dependencies(actor, fp)
                for actor in qualitylevels.findall('inline'):
                    self.append_actor_dependencies(actor, fp)

    def add_variants(self):
        self.logger.info("Loading variants...")
        for (fp, ffp) in sorted(self.find_files('art/variants', 'xml')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            variant = ElementTree.parse(ffp).getroot()
            self.append_variant_dependencies(variant, fp)

    def add_art(self):
        self.logger.info("Loading art files...")
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/textures/particles', *self.supportedTextureFormats)])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/textures/terrain', *self.supportedTextureFormats)])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/textures/skins', *self.supportedTextureFormats)])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/meshes', *self.supportedMeshesFormats)])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/animation', *self.supportedAnimationFormats)])


    def add_materials(self):
        self.logger.info("Loading materials...")
        for (fp, ffp) in sorted(self.find_files('art/materials', 'xml')):
            self.files.append(str(fp))
            material_elem = ElementTree.parse(ffp).getroot()
            for alternative in material_elem.findall('alternative'):
                material = alternative.get('material')
                if material:
                    self.deps.append((str(fp), f'art/materials/{material}'))

    def add_particles(self):
        self.logger.info("Loading particles...")
        for (fp, ffp) in sorted(self.find_files('art/particles', 'xml')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            particle = ElementTree.parse(ffp).getroot()
            texture = particle.find('texture')
            if texture:
                self.deps.append((str(fp), texture.text))

    def add_soundgroups(self):
        self.logger.info("Loading sound groups...")
        for (fp, ffp) in sorted(self.find_files('audio', 'xml')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            sound_group = ElementTree.parse(ffp).getroot()
            path = sound_group.find('Path').text.rstrip('/')
            for sound in sound_group.findall('Sound'):
                self.deps.append((str(fp), f'{path}/{sound.text}'))

    def add_audio(self):
        self.logger.info("Loading audio files...")
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('audio/', self.supportedAudioFormats)])


    def add_gui_object_repeat(self, obj, fp):
        for repeat in obj.findall('repeat'):
            for sub_obj in repeat.findall('object'):
                # TODO: look at sprites, styles, etc
                self.add_gui_object_include(sub_obj, fp)
            for sub_obj in repeat.findall('objects'):
                # TODO: look at sprites, styles, etc
                self.add_gui_object_include(sub_obj, fp)

            self.add_gui_object_include(repeat, fp)

    def add_gui_object_include(self, obj, fp):
        for include in obj.findall('include'):
            included_file = include.get('file')
            if included_file:
                self.deps.append((str(fp), f'{included_file}'))

    def add_gui_object(self, parent, fp):
        if parent is None:
            return

        for obj in parent.findall('object'):
            # TODO: look at sprites, styles, etc
            self.add_gui_object_repeat(obj, fp)
            self.add_gui_object_include(obj, fp)
            self.add_gui_object(obj, fp)
        for obj in parent.findall('objects'):
            # TODO: look at sprites, styles, etc
            self.add_gui_object_repeat(obj, fp)
            self.add_gui_object_include(obj, fp)
            self.add_gui_object(obj, fp)


    def add_gui_xml(self):
        self.logger.info("Loading GUI XML...")
        for (fp, ffp) in sorted(self.find_files('gui', 'xml')):
            self.files.append(str(fp))
            # GUI page definitions are assumed to be named page_[something].xml and alone in that.
            if match(r".*[\\\/]page(_[^.\/\\]+)?\.xml$", str(fp)):
                self.roots.append(str(fp))
                root_xml = ElementTree.parse(ffp).getroot()
                for include in root_xml.findall('include'):
                    # If including an entire directory, find all the *.xml files
                    if include.text.endswith('/'):
                        self.deps.extend([(str(fp), str(sub_fp)) for (sub_fp, sub_ffp) in self.find_files(f'gui/{include.text}', 'xml')])
                    else:
                        self.deps.append((str(fp), f'gui/{include.text}'))
            else:
                xml = ElementTree.parse(ffp)
                root_xml = xml.getroot()
                name = root_xml.tag
                self.roots.append(str(fp))
                if name in ('objects', 'object'):
                    for script in root_xml.findall('script'):
                        if script.get('file'):
                            self.deps.append((str(fp), script.get('file')))
                        if script.get('directory'):
                            # If including an entire directory, find all the *.js files
                            self.deps.extend([(str(fp), str(sub_fp)) for (sub_fp, sub_ffp) in self.find_files(script.get('directory'), 'js')])
                    self.add_gui_object(root_xml, fp)
                elif name == 'setup':
                    # TODO: look at sprites, styles, etc
                    pass
                elif name == 'styles':
                    for style in root_xml.findall('style'):
                        if(style.get('sound_opened')):
                            self.deps.append((str(fp), f"{style.get('sound_opened')}"))
                        if(style.get('sound_closed')):
                            self.deps.append((str(fp), f"{style.get('sound_closed')}"))
                        if(style.get('sound_selected')):
                            self.deps.append((str(fp), f"{style.get('sound_selected')}"))
                        if(style.get('sound_disabled')):
                            self.deps.append((str(fp), f"{style.get('sound_disabled')}"))
                    # TODO: look at sprites, styles, etc
                    pass
                elif name == 'sprites':
                    for sprite in root_xml.findall('sprite'):
                        for image in sprite.findall('image'):
                            if image.get('texture'):
                                self.deps.append((str(fp), f"art/textures/ui/{image.get('texture')}"))
                else:
                    bio = BytesIO()
                    xml.write(bio)
                    bio.seek(0)
                    raise ValueError(f"Unexpected GUI XML root element '{name}':\n{bio.read().decode('ascii')}")

    def add_gui_data(self):
        self.logger.info("Loading GUI data...")
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('gui', 'js')])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/textures/ui', *self.supportedTextureFormats)])
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('art/textures/selection', *self.supportedTextureFormats)])

    def add_civs(self):
        self.logger.info("Loading civs...")
        for (fp, ffp) in sorted(self.find_files('simulation/data/civs', 'json')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            with open(ffp, encoding='utf-8') as f:
                civ = load(f)
            for music in civ.get('Music', []):
                self.deps.append((str(fp), f"audio/music/{music['File']}"))

    def add_tips(self):
        self.logger.info("Loading tips...")
        for (fp, ffp) in sorted(self.find_files('gui/text/tips', 'txt')):
            relative_path = str(fp)
            self.files.append(relative_path)
            self.roots.append(relative_path)
            self.deps.append((relative_path, f"art/textures/ui/loading/tips/{basename(relative_path).split('.')[0]}.png"))


    def add_rms(self):
        self.logger.info("Loading random maps...")
        self.files.extend([str(fp) for (fp, ffp) in self.find_files('maps/random', 'js')])
        for (fp, ffp) in sorted(self.find_files('maps/random', 'json')):
            if str(fp).startswith('maps/random/rmbiome'):
                continue
            self.files.append(str(fp))
            self.roots.append(str(fp))
            with open(ffp, encoding='utf-8') as f:
                randmap = load(f)
            settings = randmap.get('settings', {})
            if settings.get('Script', None):
                self.deps.append((str(fp), f"maps/random/{settings['Script']}"))
            # Map previews
            if settings.get('Preview', None):
                self.deps.append((str(fp), f'art/textures/ui/session/icons/mappreview/{settings["Preview"]}'))

    def add_techs(self):
        self.logger.info("Loading techs...")
        for (fp, ffp) in sorted(self.find_files('simulation/data/technologies', 'json')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            with open(ffp, encoding='utf-8') as f:
                tech = load(f)
            if tech.get('icon', None):
                self.deps.append((str(fp), f"art/textures/ui/session/portraits/technologies/{tech['icon']}"))
            if tech.get('supersedes', None):
                self.deps.append((str(fp), f"simulation/data/technologies/{tech['supersedes']}.json"))

    def add_terrains(self):
        self.logger.info("Loading terrains...")
        for (fp, ffp) in sorted(self.find_files('art/terrains', 'xml')):
            # ignore terrains.xml
            if str(fp).endswith('terrains.xml'):
                continue
            self.files.append(str(fp))
            self.roots.append(str(fp))
            terrain = ElementTree.parse(ffp).getroot()
            for texture in terrain.find('textures').findall('texture'):
                if texture.get('file'):
                    self.deps.append((str(fp), f"art/textures/terrain/{texture.get('file')}"))
            if terrain.find('material') is not None:
                material = terrain.find('material').text
                self.deps.append((str(fp), f"art/materials/{material}"))

    def add_auras(self):
        self.logger.info("Loading auras...")
        for (fp, ffp) in sorted(self.find_files('simulation/data/auras', 'json')):
            self.files.append(str(fp))
            self.roots.append(str(fp))
            with open(ffp, encoding='utf-8') as f:
                aura = load(f)
            if aura.get('overlayIcon', None):
                self.deps.append((str(fp), aura['overlayIcon']))
            range_overlay = aura.get('rangeOverlay', {})
            for prop in ('lineTexture', 'lineTextureMask'):
                if range_overlay.get(prop, None):
                    self.deps.append((str(fp), f"art/textures/selection/{range_overlay[prop]}"))

    def check_deps(self):
        self.logger.info("Looking for missing files...")
        uniq_files = set(self.files)
        uniq_files = [r.replace(sep, '/') for r in uniq_files]
        lower_case_files = {f.lower(): f for f in uniq_files}
        reverse_deps = dict()
        for parent, dep in self.deps:
            if sep != '/':
                parent = parent.replace(sep, '/')
                dep = dep.replace(sep, '/')
            if dep not in reverse_deps:
                reverse_deps[dep] = {parent}
            else:
                reverse_deps[dep].add(parent)

        for dep in sorted(reverse_deps.keys()):
            if "simulation/templates" in dep and (
                    dep.replace("templates/", "template/special/filter/") in uniq_files or
                    dep.replace("templates/", "template/mixins/") in uniq_files
                ):
                continue

            if dep in uniq_files:
                continue

            callers = [str(self.vfs_to_relative_to_mods(ref)) for ref in reverse_deps[dep]]
            self.logger.warning(f"Missing file '{dep}' referenced by: {', '.join(sorted(callers))}")
            if dep.lower() in lower_case_files:
                self.logger.warning(f"### Case-insensitive match (found '{lower_case_files[dep.lower()]}')")

    def check_unused(self):
        self.logger.info("Looking for unused files...")
        deps = dict()
        for parent, dep in self.deps:
            if sep != '/':
                parent = parent.replace(sep, '/')
                dep = dep.replace(sep, '/')

            if parent not in deps:
                deps[parent] = {dep}
            else:
                deps[parent].add(dep)

        uniq_files = set(self.files)
        uniq_files = [r.replace(sep, '/') for r in uniq_files]
        reachable = list(set(self.roots))
        reachable = [r.replace(sep, '/') for r in reachable]
        while True:
            new_reachable = []
            for r in reachable:
                new_reachable.extend([x for x in deps.get(r, {}) if x not in reachable])
            if new_reachable:
                reachable.extend(new_reachable)
            else:
                break

        for f in sorted(uniq_files):
            if any((
                f in reachable,
                'art/terrains/' in f,
                'maps/random/' in f,
            )):
                continue
            self.logger.warning(f"Unused file '{str(self.vfs_to_relative_to_mods(f))}'")


if __name__ == '__main__':
    check_ref = CheckRefs()
    check_ref.main()
