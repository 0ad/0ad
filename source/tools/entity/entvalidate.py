#!/usr/bin/env python3
from os import chdir
from pathlib import Path
from subprocess import run, CalledProcessError
from sys import exit
from xml.etree import ElementTree
from scriptlib import warn, SimulTemplateEntity, find_files


def main():
    root = Path(__file__).resolve().parents[3]
    relaxng_schema = root / 'binaries' / 'system' / 'entity.rng'
    if not relaxng_schema.exists():
        warn(f"""Relax NG schema non existant.
Please create the file {relaxng_schema.relative_to(root)}
You can do that by running 'pyrogenesis -dumpSchema' in the 'system' directory""")
        exit(1)
    if run(['xmllint', '--version'], capture_output=True).returncode != 0:
        warn("xmllint not found in your PATH, please install it (usually in libxml2 package)")
        exit(2)
    vfs_root = root / 'binaries' / 'data' / 'mods'
    simul_templates_path = Path('simulation/templates')
    simul_template_entity = SimulTemplateEntity(vfs_root)
    count = 0
    failed = 0
    for fp, _ in sorted(find_files(vfs_root, ['public'], 'simulation/templates', 'xml')):
        if fp.stem.startswith('template_'):
            continue
        print(f"# {fp}...")
        count += 1
        entity = simul_template_entity.load_inherited(simul_templates_path, str(fp.relative_to(simul_templates_path)), ['public'])
        xmlcontent = ElementTree.tostring(entity, encoding='unicode')
        try:
            run(['xmllint', '--relaxng', str(relaxng_schema.resolve()), '-'], input=xmlcontent, capture_output=True, text=True, check=True)
        except CalledProcessError as e:
            failed += 1
            print(e.stderr)
            print(e.stdout)
    print(f"\nTotal: {count}; failed: {failed}")


if __name__ == '__main__':
    chdir(Path(__file__).resolve().parent)
    main()
