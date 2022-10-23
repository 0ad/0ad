from collections import Counter
from decimal import Decimal
from re import split
from xml.etree import ElementTree
from os.path import exists

class SimulTemplateEntity:
    def __init__(self, vfs_root, logger):
        self.vfs_root = vfs_root
        self.logger = logger

    def get_file(self, base_path, vfs_path, mod):
        default_path = self.vfs_root / mod / base_path
        file = (default_path / "special" / "filter" / vfs_path).with_suffix('.xml')
        if not exists(file):
            file = (default_path / "mixins" / vfs_path).with_suffix('.xml')
        if not exists(file):
            file = (default_path / vfs_path).with_suffix('.xml')
        return file

    def get_main_mod(self, base_path, vfs_path, mods):
        for mod in mods:
            fp = self.get_file(base_path, vfs_path, mod)
            if fp.exists():
                main_mod = mod
                break
        else:
            # default to first mod
            # it should then not exist
            # it will raise an exception when trying to read it
            main_mod = mods[0]
        return main_mod

    def apply_layer(self, base_tag, tag):
        """
        apply tag layer to base_tag
        """
        if tag.get('datatype') == 'tokens':
            base_tokens = split(r'\s+', base_tag.text or '')
            tokens = split(r'\s+', tag.text or '')
            final_tokens = base_tokens.copy()
            for token in tokens:
                if token.startswith('-'):
                    token_to_remove = token[1:]
                    if token_to_remove in final_tokens:
                        final_tokens.remove(token_to_remove)
                elif token not in final_tokens:
                    final_tokens.append(token)
            base_tag.text = ' '.join(final_tokens)
            base_tag.set("datatype", "tokens")
        elif tag.get('op'):
            op = tag.get('op')
            op1 = Decimal(base_tag.text or '0')
            op2 = Decimal(tag.text or '0')
            # Try converting to integers if possible, to pass validation.
            if op == 'add':
                base_tag.text = str(int(op1 + op2) if int(op1 + op2) == op1 + op2 else op1 + op2)
            elif op == 'mul':
                base_tag.text = str(int(op1 * op2) if int(op1 * op2) == op1 * op2 else op1 * op2)
            elif op == 'mul_round':
                base_tag.text = str(round(op1 * op2))
            else:
                raise ValueError(f"Invalid operator '{op}'")
        else:
            base_tag.text = tag.text
            for prop in tag.attrib:
                if prop not in ('disable', 'replace', 'parent', 'merge'):
                    base_tag.set(prop, tag.get(prop))
        for child in tag:
            base_child = base_tag.find(child.tag)
            if 'disable' in child.attrib:
                if base_child is not None:
                    base_tag.remove(base_child)
            elif ('merge' not in child.attrib) or (base_child is not None):
                if 'replace' in child.attrib and base_child is not None:
                    base_tag.remove(base_child)
                    base_child = None
                if base_child is None:
                    base_child = ElementTree.Element(child.tag)
                    base_tag.append(base_child)
                self.apply_layer(base_child, child)
                if 'replace' in base_child.attrib:
                    del base_child.attrib['replace']

    def load_inherited(self, base_path, vfs_path, mods):
        entity = self._load_inherited(base_path, vfs_path, mods)
        entity[:] = sorted(entity[:], key=lambda x: x.tag)
        return entity

    def _load_inherited(self, base_path, vfs_path, mods, base=None):
        """
        vfs_path should be relative to base_path in a mod
        """
        if '|' in vfs_path:
            paths = vfs_path.split("|", 1)
            base = self._load_inherited(base_path, paths[1], mods, base)
            base = self._load_inherited(base_path, paths[0], mods, base)
            return base

        main_mod = self.get_main_mod(base_path, vfs_path, mods)
        fp = self.get_file(base_path, vfs_path, main_mod)
        layer = ElementTree.parse(fp).getroot()
        for el in layer.iter():
            children = [x.tag for x in el]
            duplicates = [x for x, c in Counter(children).items() if c > 1]
            if duplicates:
                for dup in duplicates:
                    self.logger.warning(f"Duplicate child node '{dup}' in tag {el.tag} of {fp}")
        if layer.get('parent'):
            parent = self._load_inherited(base_path, layer.get('parent'), mods, base)
            self.apply_layer(parent, layer)
            return parent
        else:
            if not base:
                return layer
            else:
                self.apply_layer(base, layer)
                return base


def find_files(vfs_root, mods, vfs_path, *ext_list):
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
