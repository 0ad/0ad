#!/usr/bin/env python3
from os import chdir
from pathlib import Path
from re import split
from subprocess import run
from sys import exit
from scriptlib import warn, SimulTemplateEntity, find_files


def find_entities(vfs_root):
    base = vfs_root / 'public' / 'simulation' / 'templates'
    return [str(fp.relative_to(base).with_suffix('')) for (_, fp) in find_files(vfs_root, ['public'], 'simulation/templates', 'xml')]


def main():
    vfs_root = Path(__file__).resolve().parents[3] / 'binaries' / 'data' / 'mods'
    simul_templates_path = Path('simulation/templates')
    simul_template_entity = SimulTemplateEntity(vfs_root)
    with open('creation.dot', 'w') as dot_f:
        dot_f.write('digraph G {\n')
        files = sorted(find_entities(vfs_root))
        for f in files:
            if f.startswith('template_'):
                continue
            print(f"# {f}...")
            entity = simul_template_entity.load_inherited(simul_templates_path, f, ['public'])
            if entity.find('Builder') is not None and entity.find('Builder').find('Entities') is not None:
                entities = entity.find('Builder').find('Entities').text.replace('{civ}', entity.find('Identity').find('Civ').text)
                builders = split(r'\s+', entities.strip())
                for builder in builders:
                    if Path(builder) in files:
                        warn(f"Invalid Builder reference: {f} -> {builder}")
                    dot_f.write(f'"{f}" -> "{builder}" [color=green];\n')
            if entity.find('TrainingQueue') is not None and entity.find('TrainingQueue').find('Entities') is not None:
                entities = entity.find('TrainingQueue').find('Entities').text.replace('{civ}', entity.find('Identity').find('Civ').text)
                training_queues = split(r'\s+', entities.strip())
                for training_queue in training_queues:
                    if Path(training_queue) in files:
                        warn(f"Invalid TrainingQueue reference: {f} -> {training_queue}")
                    dot_f.write(f'"{f}" -> "{training_queue}" [color=blue];\n')
        dot_f.write('}\n')
    if run(['dot', '-V'], capture_output=True).returncode == 0:
        exit(run(['dot', '-Tpng', 'creation.dot', '-o', 'creation.png'], text=True).returncode)


if __name__ == '__main__':
    chdir(Path(__file__).resolve().parent)
    main()
