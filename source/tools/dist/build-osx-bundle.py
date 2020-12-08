#!/usr/bin/python3
"""
Builds the OSX bundle from existing elements.
App bundles are intended to be self-contained and portable.
An SDK is required, usually included with Xcode. The SDK ensures
that only those system libraries are used which are available on
the chosen target and compatible systems.

This is Python because plistlib is extremely strict about what it accepts
and it's used by dmgbuild, and saving the Plist doesn't really work otherwise.
"""
import argparse
import datetime
import glob
import os
import plistlib
import shutil

import dmgbuild

parser = argparse.ArgumentParser()
parser.add_argument('bundle_version', help='Bundle version')
parser.add_argument('--min_osx', help='Minimum supported OSX version',
    default='10.12')
parser.add_argument('--bundle_identifier', help='Bundle identifier',
    default='com.wildfiregames.0ad')
args = parser.parse_args()

BUNDLE_DMG_NAME = "0 A.D."
BUNDLE_OUTPUT = "0 A.D..app"
BUNDLE_CONTENTS = BUNDLE_OUTPUT + "/Contents"
BUNDLE_BIN = BUNDLE_CONTENTS + "/MacOS"
BUNDLE_RESOURCES = BUNDLE_CONTENTS + "/Resources"
BUNDLE_FRAMEWORKS = BUNDLE_CONTENTS + "/Frameworks"
BUNDLE_PLUGINS = BUNDLE_CONTENTS + "/PlugIns"
BUNDLE_SHAREDSUPPORT = BUNDLE_CONTENTS + "/SharedSupport"

BUNDLE_IDENTIFIER = args.bundle_identifier
BUNDLE_VERSION = args.bundle_version
BUNDLE_MIN_OSX_VERSION = args.min_osx

print("Creating bundle directories")

shutil.rmtree(BUNDLE_OUTPUT, ignore_errors=True)
os.makedirs(BUNDLE_BIN)
os.makedirs(BUNDLE_FRAMEWORKS)
os.makedirs(BUNDLE_PLUGINS)
os.makedirs(BUNDLE_RESOURCES)
os.makedirs(BUNDLE_SHAREDSUPPORT)

print("Copying binaries")

# Only pyrogenesis for now, until we find a way to load
#     multiple binaries from one app bundle
# TODO: Would be nicer if we could set this path in premake
shutil.copy("binaries/system/pyrogenesis", BUNDLE_BIN)

print("Copying libs")
shutil.copy("binaries/system/libAtlasUI.dylib", BUNDLE_FRAMEWORKS)
shutil.copy("binaries/system/libCollada.dylib", BUNDLE_FRAMEWORKS)

print("Copying archived game data")
for mod in glob.glob("archives/*/"):
    print(f"Copying {mod}")
    shutil.copytree(mod, BUNDLE_RESOURCES + "/data/mods/" + mod.replace("archives/", ""))

print("Copying non-archived game data")
shutil.copytree("binaries/data/config", BUNDLE_RESOURCES + "/data/config")
shutil.copytree("binaries/data/l10n", BUNDLE_RESOURCES + "/data/l10n")
shutil.copytree("binaries/data/tools", BUNDLE_RESOURCES + "/data/tools")
# Remove the dev.cfg file or 0 A.D. will assume it's running a dev copy.
os.unlink(BUNDLE_RESOURCES + "/data/config/dev.cfg")

shutil.copy("build/resources/0ad.icns", BUNDLE_RESOURCES)
shutil.copy("build/resources/InfoPlist.strings", BUNDLE_RESOURCES)

print("Copying readmes")
# TODO: Also want copies in the DMG - decide on layout
for file in glob.glob("*.txt"):
    shutil.copy(file, BUNDLE_RESOURCES)
shutil.copy("libraries/LICENSE.txt", BUNDLE_RESOURCES + "/LIB_LICENSE.txt")

print("Creating Info.plist")

with open(BUNDLE_CONTENTS + "/Info.plist", 'wb') as f:
    plistlib.dump({
        'CFBundleName': '0 A.D.',
        'CFBundleIdentifier': BUNDLE_IDENTIFIER,
        'CFBundleVersion': BUNDLE_VERSION,
        'CFBundlePackageType': 'APPL',
        'CFBundleSignature': 'none',
        'CFBundleExecutable': 'pyrogenesis',
        'CFBundleShortVersionString': BUNDLE_VERSION,
        'CFBundleDevelopmentRegion': 'English',
        'CFBundleInfoDictionaryVersion': '6.0',
        'CFBundleIconFile': '0ad',
        'LSHasLocalizedDisplayName': True,
        'LSMinimumSystemVersion': BUNDLE_MIN_OSX_VERSION,
        'NSHumanReadableCopyright': f'Copyright © {datetime.datetime.now().year} Wildfire Games',
    }, f)

print("Creating .dmg")

# Package the app into a dmg
dmgbuild.build_dmg(
    filename=BUNDLE_DMG_NAME + ".dmg",
    volume_name=BUNDLE_DMG_NAME,
    settings_file="source/tools/dist/dmgbuild-settings.py",
    defines={
        "app": BUNDLE_OUTPUT,
        "background": "build/resources/dmgbackground.png",
        "icon": "build/resources/0ad.icns"
    })

print(f"Bundle complete! Located in {BUNDLE_OUTPUT}, compressed as {BUNDLE_DMG_NAME}.dmg.")
