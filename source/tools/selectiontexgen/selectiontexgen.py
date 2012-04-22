"""
Generates basic square and circle selection overlay textures by parsing all the entity XML files and reading
their Footprint components.

For usage, invoke this script with --help.
"""

# This script uses PyCairo for plotting, since PIL (Python Imaging Library) is absolutely horrible. On Linux,
# this should be merely a matter of installing a package (e.g. 'python-cairo' for Debian/Ubuntu), but on Windows
# it's kind of tricky and requires some Google-fu. Fortunately, I have saved the working instructions below:
# 
# Grab a Win32 binary from http://ftp.gnome.org/pub/GNOME/binaries/win32/pycairo/1.8/ and install PyCairo using 
# the installer. The installer extracts the necessary files into Lib\site-packages\cairo within the folder where 
# Python is installed. There are some extra DLLs which are required to make Cairo work, so we have to get these 
# as well.
# 
# Head to http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/ and get the binary versions of Cairo 
# (cairo_1.8.10-3_win32.zip at the time of writing), Fontconfig (fontconfig_2.8.0-2_win32.zip), Freetype 
# (freetype_2.4.4-1_win32.zip), Expat (expat_2.0.1-1_win32.zip), libpng (libpng_1.4.3-1_win32.zip) and zlib 
# (zlib_1.2.5-2_win32.zip). Version numbers may vary, so be adaptive! Each ZIP file will contain a bin subfolder 
# with a DLL file in it. Put the following DLLs in Lib\site-packages\cairo within your Python installation:
#
#    freetype6.dll (from freetype_2.4.4-1_win32.zip)
#    libcairo-2.dll (from cairo_1.8.10-3_win32.zip)
#    libexpat-1.dll (from expat_2.0.1-1_win32.zip)
#    libfontconfig-1.dll (from fontconfig_2.8.0-2_win32.zip)
#    libpng14-14.dll (from libpng_1.4.3-1_win32.zip)
#    zlib1.dll (from zlib_1.2.5-2_win32.zip).
#
# Should be all set now.

import optparse
import sys, os
import math
import operator
import cairo					# Requires PyCairo (see notes above)
from os.path import *
from xml.dom import minidom

def geqPow2(x):
	"""Returns the smallest power of two that's equal to or greater than x"""
	return int(2**math.ceil(math.log(x, 2)))

def generateSelectionTexture(shape, textureW, textureH, outerStrokeW, innerStrokeW, outputDir):
	
	outputBasename = "%dx%d" % (textureW, textureH)
	
	# size of the image canvas containing the texture (may be larger to ensure power-of-two dimensions)
	canvasW = geqPow2(textureW)
	canvasH = geqPow2(textureH)
	
	# draw texture
	texture = cairo.ImageSurface(cairo.FORMAT_ARGB32, canvasW, canvasH)
	textureMask = cairo.ImageSurface(cairo.FORMAT_RGB24, canvasW, canvasH)
	
	ctxTexture = cairo.Context(texture)
	ctxTextureMask = cairo.Context(textureMask)
	
	# fill entire image with transparent pixels
	ctxTexture.set_source_rgba(1.0, 1.0, 1.0, 0.0)		# transparent
	ctxTexture.rectangle(0, 0, textureW, textureH)			
	ctxTexture.fill()									# fill current path
	
	ctxTextureMask.set_source_rgb(0.0, 0.0, 0.0)		# black
	ctxTextureMask.rectangle(0, 0, canvasW, canvasH)	# (!)
	ctxTextureMask.fill()
	
	pasteX = (canvasW - textureW)//2					# integer division, floored result
	pasteY = (canvasH - textureH)//2					# integer division, floored result
	ctxTexture.translate(pasteX, pasteY)				# translate all drawing so that the result is centered
	ctxTextureMask.translate(pasteX, pasteY)
	
	# outer stroke width should always be >= inner stroke width, but let's play it safe
	maxStrokeW = max(outerStrokeW, innerStrokeW)
	
	if shape == "square":
		
		rectW = textureW
		rectH = textureH
		
		# draw texture (4px white outline, then overlay a 2px black outline)
		ctxTexture.rectangle(maxStrokeW/2, maxStrokeW/2, rectW - maxStrokeW, rectH - maxStrokeW)
		ctxTexture.set_line_width(outerStrokeW)
		ctxTexture.set_source_rgba(1.0, 1.0, 1.0, 1.0)	# white
		ctxTexture.stroke_preserve()					# stroke and maintain path
		ctxTexture.set_line_width(innerStrokeW)
		ctxTexture.set_source_rgba(0.0, 0.0, 0.0, 1.0)	# black
		ctxTexture.stroke()								# stroke and clear path
		
		# draw mask (2px white)
		ctxTextureMask.rectangle(maxStrokeW/2, maxStrokeW/2, rectW - maxStrokeW, rectH - maxStrokeW)
		ctxTextureMask.set_line_width(innerStrokeW)
		ctxTextureMask.set_source_rgb(1.0, 1.0, 1.0)
		ctxTextureMask.stroke()
		
	elif shape == "circle":
		
		centerX = textureW//2
		centerY = textureH//2
		radius = textureW//2 - maxStrokeW/2				# allow for the strokes to fit 
		
		# draw texture
		ctxTexture.arc(centerX, centerY, radius, 0, 2*math.pi)
		ctxTexture.set_line_width(outerStrokeW)
		ctxTexture.set_source_rgba(1.0, 1.0, 1.0, 1.0)	# white
		ctxTexture.stroke_preserve()					# stroke and maintain path
		ctxTexture.set_line_width(innerStrokeW)
		ctxTexture.set_source_rgba(0.0, 0.0, 0.0, 1.0)	# black
		ctxTexture.stroke()
		
		# draw mask
		ctxTextureMask.arc(centerX, centerY, radius, 0, 2*math.pi)
		ctxTextureMask.set_line_width(innerStrokeW)
		ctxTextureMask.set_source_rgb(1.0, 1.0, 1.0)
		ctxTextureMask.stroke()
	
	finalOutputDir = outputDir + "/" + shape
	if not isdir(finalOutputDir):
		os.makedirs(finalOutputDir)
	
	print "Generating " + os.path.normcase(finalOutputDir + "/" + outputBasename + ".png")
	
	texture.write_to_png(finalOutputDir + "/" + outputBasename + ".png")
	textureMask.write_to_png(finalOutputDir + "/" + outputBasename + "_mask.png")


def generateSelectionTextures(xmlTemplateDir, outputDir, outerStrokeScale, innerStrokeScale, snapSizes = False):
	
	# recursively list XML files
	xmlFiles = []
	
	for dir, subdirs, basenames in os.walk(xmlTemplateDir):
		for basename in basenames:
			filename = join(dir, basename)
			if filename[-4:] == ".xml":
				xmlFiles.append(filename)
	
	textureTypesRaw = set()		# set of (type, w, h) tuples (so we can eliminate duplicates)
	
	# parse the XML files, and look for <Footprint> nodes that are a child of <Entity> and
	# that do not have the disable attribute defined
	for xmlFile in xmlFiles:
		xmlDoc = minidom.parse(xmlFile)
		rootNode = xmlDoc.childNodes[0]
		
		# we're only interested in entity templates
		if not rootNode.nodeName == "Entity":
			continue
		
		# check if this entity has a footprint definition
		rootChildNodes = [n for n in rootNode.childNodes if n.localName is not None] # remove whitespace text nodes
		footprintNodes = filter(lambda x: x.localName == "Footprint", rootChildNodes)
		if not len(footprintNodes) == 1:
			continue
		
		footprintNode = footprintNodes[0]
		if footprintNode.hasAttribute("disable"):
			continue
		
		# parse the footprint declaration
		# Footprints can either have either one of these children:
		# <Circle radius="xx.x" />
		# <Square width="xx.x" depth="xx.x"/>
		# There's also a <Height> node, but we don't care about it here.
		
		squareNodes = footprintNode.getElementsByTagName("Square")
		circleNodes = footprintNode.getElementsByTagName("Circle")
		
		numSquareNodes = len(squareNodes)
		numCircleNodes = len(circleNodes)
		
		if not (numSquareNodes + numCircleNodes == 1):
			print "Invalid Footprint definition: insufficient or too many Square and/or Circle definitions in %s" % xmlFile
		
		texShape = None
		texW = None		# in world-space units
		texH = None		# in world-space units
		
		if numSquareNodes == 1:
			texShape = "square"
			texW = float(squareNodes[0].getAttribute("width"))
			texH = float(squareNodes[0].getAttribute("depth"))
		
		elif numCircleNodes == 1:
			texShape = "circle"
			texW = float(circleNodes[0].getAttribute("radius"))
			texH = texW
		
		textureTypesRaw.add((texShape, texW, texH))
	
	# endfor xmlFiles
	
	print "Found:     %d footprints (%d square, %d circle)" % (
		len(textureTypesRaw),
		len([x for x in textureTypesRaw if x[0] == "square"]),
		len([x for x in textureTypesRaw if x[0] == "circle"])
	)
	
	textureTypes = set()
	
	for type, w, h in textureTypesRaw:
		if snapSizes:
			# "snap" texture sizes to close-enough neighbours that will still look good enough so we can get away with fewer 
			# actual textures than there are unique footprint outlines 
			w = 1*math.ceil(w/1) 	# round up to the nearest world-space unit
			h = 1*math.ceil(h/1)	# round up to the nearest world-space unit
			
		textureTypes.add((type, w, h))
	
	if snapSizes:
		print "Reduced: %d footprints (%d square, %d circle)" % (
			len(textureTypes),
			len([x for x in textureTypes if x[0] == "square"]),
			len([x for x in textureTypes if x[0] == "circle"])
		)
	
	# create list from texture types set (so we can sort and have prettier output)
	textureTypes = sorted(list(textureTypes), key=operator.itemgetter(0,1,2))		# sort by the first tuple element, then by the second, then the third
	
	# ------------------------------------------------------------------------------------
	# compute the size of the actual texture we want to generate (in px)
	
	scale = 8		# world-space-units-to-pixels scale
	for type, w, h in textureTypes:
		
		# if we have a circle, update the w and h so that they're the full width and height of the texture 
		# and not just the radius
		if type == "circle":
			assert w == h
			w *= 2
			h *= 2
		
		w = int(math.ceil(w*scale))
		h = int(math.ceil(h*scale))
		
		# apply a minimum size for really small textures
		w = max(24, w)
		h = max(24, h)
		
		generateSelectionTexture(type, w, h, w/outerStrokeScale, innerStrokeScale * (w/outerStrokeScale), outputDir)


if __name__ == "__main__":
	
	parser = optparse.OptionParser(usage="Usage: %prog [filenames]")
	
	parser.add_option("--template-dir",        type="str",  default=None,  help="Path to simulation template XML definition folder. Will be searched recursively for templates containing Footprint definitions. If not specified and this script is run from its directory, it will be automatically determined.")
	parser.add_option("--output-dir",          type="str",  default=".",   help="Output directory. Will be created if it does not already exist. Defaults to the current directory.")
	parser.add_option("--oss", "--outer-stroke-scale",  type="float",  default=12.0, dest="outer_stroke_scale", metavar="SCALE",      help="Width of the outer (white) stroke, as a divisor of each generated texture's width. Defaults to 12. Larger values produce thinner overall outlines.")
	parser.add_option("--iss", "--inner-stroke-scale",  type="float",  default=0.5,  dest="inner_stroke_scale", metavar="PERCENTAGE", help="Width of the inner (black) stroke, as a percentage of the outer stroke's calculated width. Must be between 0 and 1. Higher values produce thinner black/player color strokes inside the surrounding outer white stroke. Defaults to 0.5.")
	
	(options, args) = parser.parse_args()
	
	templateDir = options.template_dir
	if templateDir is None:
		
		scriptDir = dirname(abspath(__file__))
		
		# 'autodetect' location if run from its own dir
		if normcase(scriptDir).replace('\\', '/').endswith("source/tools/selectiontexgen"):
			templateDir = "../../../binaries/data/mods/public/simulation/templates"
		else:
			print "No template dir specified; use the --template-dir command line argument."
			sys.exit()
	
	# check if the template dir exists
	templateDir = abspath(templateDir)
	if not isdir(templateDir):
		print "No such template directory: %s" % templateDir
		sys.exit()
	
	# check if the output dir exists, create it if needed
	outputDir = abspath(options.output_dir)
	print outputDir
	if not isdir(outputDir):
		print "Creating output directory: %s" % outputDir
		os.makedirs(outputDir)
	
	print "Template directory:\t%s" % templateDir
	print "Output directory:  \t%s" % outputDir
	print "------------------------------------------------"
	
	generateSelectionTextures(
		templateDir,
		outputDir,
		max(0.0, options.outer_stroke_scale),
		min(1.0, max(0.0, options.inner_stroke_scale)),
	)