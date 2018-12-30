#!/usr/local/bin/python3
#
# Script for symlinking OS X dependencies for 0 A.D. using brew
# Since OSX always prefer dynamic libraries, this specifically symlinks static versions
#
# DOC to write.
#
# --------------------------------------------------------------
# Library versions for ease of updating:
ZLIB_VERSION="zlib-1.2.11"
CURL_VERSION="curl-7.59.0"
ICONV_VERSION="libiconv-1.15"
XML2_VERSION="libxml2-2.9.8"
SDL2_VERSION="SDL2-2.0.5"
BOOST_VERSION="boost_1_64_0"
# NOTE: remember to also update LIB_URL below when changing version
WXWIDGETS_VERSION="wxWidgets-3.0.4.1"
# libpng was included as part of X11 but that's removed from Mountain Lion
# (also the Snow Leopard version was ancient 1.2)
PNG_VERSION="libpng-1.6.34"
OGG_VERSION="libogg-1.3.3"
VORBIS_VERSION="libvorbis-1.3.6"
# gloox is necessary for multiplayer lobby
GLOOX_VERSION="gloox-1.0.20"
# gloox requires GnuTLS, GnuTLS requires Nettle and GMP
GMP_VERSION="gmp-6.1.2"
NETTLE_VERSION="nettle-3.4"
GNUTLS_VERSION="gnutls-3.5.19"
GLOOX_VERSION="gloox-1.0.20"
# NSPR is necessary for threadsafe Spidermonkey
NSPR_VERSION="4.15"
# OS X only includes part of ICU, and only the dylib
# NOTE: remember to also update LIB_URL below when changing version
ICU_VERSION="icu4c-59_1"
ENET_VERSION="enet-1.3.13"
MINIUPNPC_VERSION="miniupnpc-2.0.20180222"
SODIUM_VERSION="libsodium-1.0.16"
# --------------------------------------------------------------
# Bundled with the game:
# * SpiderMonkey 38
# * NVTT
# * FCollada
# --------------------------------------------------------------
# Provided by OS X:
# * OpenAL
# * OpenGL
# --------------------------------------------------------------

libs_to_fetch = {
  'boost':'boost'
  ,'enet':'enet'
  ,'nettle':'nettle'
  ,'gmp':'gmp'
  ,'gnutls':'gnutls'
  ,'libiconv':'iconv'
  ,'icu4c':'icu'
  ,'libogg':'libogg'
  ,'libpng':'libpng'
  ,'libsodium':'libsodium'
  ,'libxml2':'libxml2'
  ,'openssl':'ssl'
  ,'miniupnpc':'miniupnpc'
  ,'nspr':'nspr'
  ,'sdl2':'sdl2'
  ,'libvorbis':'vorbis'
  ,'zlib':'zlib'
  ,'wxmac':'wxwidgets'
}

import os
import glob
from distutils.version import LooseVersion
from pathlib import Path

BREW_DIR = '/usr/local/Cellar/'
LIBS_PATH = os.path.dirname(os.path.realpath(__file__))  + '/'

def safe_unlink(p):
  if Path(p).is_symlink():
    os.unlink(p)

def safe_symlink(src, dest):
  safe_unlink(dest)
  if os.path.exists(src):
    os.symlink(src, dest)

for (brew_lib_name, zeroad_lib_name) in libs_to_fetch.items():
  versions = os.listdir(BREW_DIR + brew_lib_name)
  versions.sort(key=LooseVersion)
  brew_path = BREW_DIR + brew_lib_name + '/' + versions[-1]

  print(LIBS_PATH + zeroad_lib_name)
  out_path = LIBS_PATH + zeroad_lib_name
  os.makedirs(out_path, exist_ok = True)

  safe_symlink(brew_path + '/include', out_path + '/include')
  safe_symlink(brew_path + '/bin', out_path + '/bin')

  os.makedirs(out_path + '/lib', exist_ok = True)
  for lib in glob.glob(brew_path + '/lib/**.a'):
    safe_symlink(lib, out_path + '/lib/' + lib.split('/')[-1])
