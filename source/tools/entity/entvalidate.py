#!/usr/bin/env python3
import argparse
import logging
from pathlib import Path
import shutil
from subprocess import run, CalledProcessError
import sys
from typing import Sequence

from xml.etree import ElementTree

from scriptlib import SimulTemplateEntity, find_files

SIMUL_TEMPLATES_PATH = Path("simulation/templates")
ENTITY_RELAXNG_FNAME = "entity.rng"
RELAXNG_SCHEMA_ERROR_MSG = """Relax NG schema non existant.
Please create the file: {}
You can do that by running 'pyrogenesis -dumpSchema' in the 'system' directory
"""
XMLLINT_ERROR_MSG = ("xmllint not found in your PATH, please install it "
                     "(usually in libxml2 package)")

class SingleLevelFilter(logging.Filter):
    def __init__(self, passlevel, reject):
        self.passlevel = passlevel
        self.reject = reject

    def filter(self, record):
        if self.reject:
            return (record.levelno != self.passlevel)
        else:
            return (record.levelno == self.passlevel)

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
# create a console handler, seems nicer to Windows and for future uses
ch = logging.StreamHandler(sys.stdout)
ch.setLevel(logging.INFO)
ch.setFormatter(logging.Formatter('%(levelname)s - %(message)s'))
f1 = SingleLevelFilter(logging.INFO, False)
ch.addFilter(f1)
logger.addHandler(ch)
errorch =logging. StreamHandler(sys.stderr)
errorch.setLevel(logging.WARNING)
errorch.setFormatter(logging.Formatter('%(levelname)s - %(message)s'))
logger.addHandler(errorch)

def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate templates")
    parser.add_argument("-m", "--mod-name", required=True,
                        help="The name of the mod to validate.")
    parser.add_argument("-r", "--root", dest="vfs_root", default=Path(),
                        type=Path, help="The path to mod's root location.")
    parser.add_argument("-s", "--relaxng-schema",
                        default=Path() / ENTITY_RELAXNG_FNAME, type=Path,
                        help="The path to mod's root location.")
    parser.add_argument("-t", "--templates", nargs="*",
                        help="Optionally, a list of templates to validate.")
    parser.add_argument("-v", "--verbose",
                        help="Be verbose about the output.",  default=False)

    args = parser.parse_args(argv)

    if not args.relaxng_schema.exists():
        logging.error(RELAXNG_SCHEMA_ERROR_MSG.format(args.relaxng_schema))
        return 1

    if not shutil.which("xmllint"):
        logging.error(XMLLINT_ERROR_MSG)
        return 2

    if args.templates:
        templates = sorted([(Path(t), None) for t in args.templates])
    else:
        templates = sorted(find_files(args.vfs_root, [args.mod_name],
                                      SIMUL_TEMPLATES_PATH.as_posix(), "xml"))

    simul_template_entity = SimulTemplateEntity(args.vfs_root, logger)
    count, failed = 0, 0
    for fp, _ in templates:
        if fp.stem.startswith("template_"):
            continue

        path = fp.as_posix()
        if (path.startswith(f"{SIMUL_TEMPLATES_PATH.as_posix()}/mixins/")
                or path.startswith(
                    f"{SIMUL_TEMPLATES_PATH.as_posix()}/special/")):
            continue

        if (args.verbose):
            logger.info(f"Parsing {fp}...")
        count += 1
        entity = simul_template_entity.load_inherited(
            SIMUL_TEMPLATES_PATH,
            str(fp.relative_to(SIMUL_TEMPLATES_PATH)),
            [args.mod_name]
        )
        xmlcontent = ElementTree.tostring(entity, encoding="unicode")
        try:
            run(["xmllint", "--relaxng",
                 str(args.relaxng_schema.resolve()), "-"],
                input=xmlcontent, encoding="utf-8", capture_output=True, text=True, check=True)
        except CalledProcessError as e:
            failed += 1
            if (e.stderr):
                logger.error(e.stderr)
            if (e.stdout):
                logger.info(e.stdout)

    logger.info(f"Total: {count}; failed: {failed}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
