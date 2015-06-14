#! /usr/bin/env python3

# Copyright (c) 2015 Sanderd17
#
# Licensed under the MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import argparse
import io
import os
import struct
import sys

parser = argparse.ArgumentParser(description="Convert maps compatible with 0 A.D. version Alpha XVIII (A18) to maps compatible with version Alpha XIX (A19), or the other way around.")

parser.add_argument("--reverse", action="store_true", help="Make an A19 map compatible with A18 (note that conversion will fail if mountains are too high)")
parser.add_argument("--no-version-bump", action="store_true", help="Don't change the version number of the map")
parser.add_argument("--no-color-spelling", action="store_true", help="Don't change the spelling of color and colour")
parser.add_argument("--no-height-change", action="store_true", help="Don't change the heightmap")

parser.add_argument("files", nargs="+", help="XML file to process (use wildcards '*' to select multiple files)")
args = parser.parse_args()


HEIGHTMAP_BIT_SHIFT = 3

for xmlFile in args.files:
	pmpFile = xmlFile[:-3] + "pmp"

	print("Processing " + xmlFile + " ...")

	if os.path.isfile(pmpFile):
		with open(pmpFile, "rb") as f1, open(pmpFile + "~", "wb") as f2:
			# 4 bytes PSMP to start the file 
			f2.write(f1.read(4))

			# 4 bytes to encode the version of the file format
			version = struct.unpack("<I", f1.read(4))[0]
			if args.no_version_bump:
				f2.write(struct.pack("<I", version))
			else:
				if args.reverse:
					if version != 6:
						print("Warning: File " + pmpFile + " was not at version 6, while a negative version bump was requested.\nABORTING ...")
						continue
					f2.write(struct.pack("<I", version-1))
				else:
					if version != 5:
						print("Warning: File " + pmpFile + " was not at version 5, while a version bump was requested.\nABORTING ...")
						continue
					f2.write(struct.pack("<I", version+1))

			# 4 bytes a for file size (which shouldn't change) 
			f2.write(f1.read(4))

			# 4 bytes to encode the map size
			map_size = struct.unpack("<I", f1.read(4))[0]
			f2.write(struct.pack("<I", map_size))

			# half all heights using the shift '>>' operator
			if args.no_height_change:
				def height_transform(h):
					return h
			else:
				if args.reverse:
					def height_transform(h):
						return h << HEIGHTMAP_BIT_SHIFT
				else:
					def height_transform(h):
						return h >> HEIGHTMAP_BIT_SHIFT
					
			for i in range(0, (map_size*16+1)*(map_size*16+1)):
				height = struct.unpack("<H", f1.read(2))[0]
				f2.write(struct.pack("<H", height_transform(height)))
			
			# copy the rest of the file
			byte = f1.read(1)
			while byte != b"":
				f2.write(byte)
				byte = f1.read(1)

			f2.close()
			f1.close()

		# replace the old file, comment to see both files
		os.remove(pmpFile)
		os.rename(pmpFile + "~", pmpFile)


	if os.path.isfile(xmlFile):
		with open(xmlFile, "r") as f1, open(xmlFile + "~", "w") as f2:
			data = f1.read()

			# bump version number (rely on how Atlas formats the XML)
			if not args.no_version_bump:
				if args.reverse:
					if data.find('<Scenario version="6">') == -1:
						print("Warning: File " + xmlFile + " was not at version 6, while a negative version bump was requested.\nABORTING ...")
						sys.exit()
					else:
						data = data.replace('<Scenario version="6">', '<Scenario version="5">')
				else:
					if data.find('<Scenario version="5">') == -1:
						print("Warning: File " + xmlFile + " was not at version 5, while a version bump was requested.\nABORTING ...")
						sys.exit()
					else:
						data = data.replace('<Scenario version="5">', '<Scenario version="6">')
						

			# transform the color keys
			if not args.no_color_spelling:
				if args.reverse:
					data = data.replace("color", "colour").replace("Color", "Colour")
				else:
					data = data.replace("colour", "color").replace("Colour", "Color")
			
			f2.write(data)
			f1.close()
			f2.close()

		# replace the old file, comment to see both files
		os.remove(xmlFile)
		os.rename(xmlFile + "~", xmlFile)
