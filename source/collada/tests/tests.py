from ctypes import *
import sys
import os
import xml.etree.ElementTree as ET

binaries = '../../../binaries'

# Work out the platform-dependent library filename
dll_filename = {
	'posix': './libCollada_dbg.so',
	'nt': 'Collada_dbg.dll',
}[os.name]

# The DLL may need other DLLs which are in its directory, so set the path to that
# (Don't care about clobbering the old PATH - it doesn't have anything important)
os.environ['PATH'] = '%s/system/' % binaries

# Load the actual library
library = cdll.LoadLibrary('%s/system/%s' % (binaries, dll_filename))

def log(severity, message):
	print '[%s] %s' % (('INFO', 'WARNING', 'ERROR')[severity], message)

clog = CFUNCTYPE(None, c_int, c_char_p)(log)
	# (the CFUNCTYPE must not be GC'd, so try to keep a reference)
library.set_logger(clog)
skeleton_definitions = open('%s/data/tools/collada/skeletons.xml' % binaries).read()
library.set_skeleton_definitions(skeleton_definitions, len(skeleton_definitions))

def _convert_dae(func, filename, expected_status=0):
	output = []
	def cb(cbdata, str, len):
		output.append(string_at(str, len))

	cbtype = CFUNCTYPE(None, POINTER(None), POINTER(c_char), c_uint)
	status = func(filename, cbtype(cb), None)
	assert(status == expected_status)
	return ''.join(output)

def convert_dae_to_pmd(*args, **kwargs):
	return _convert_dae(library.convert_dae_to_pmd, *args, **kwargs)

def convert_dae_to_psa(*args, **kwargs):
	return _convert_dae(library.convert_dae_to_psa, *args, **kwargs)

def clean_dir(path):
	# Remove all files first
	try:
		for f in os.listdir(path):
			os.remove(path+'/'+f)
		os.rmdir(path)
	except OSError:
		pass # (ignore errors if files are in use)
	# Make sure the directory exists
	try:
		os.makedirs(path)
	except OSError:
		pass # (ignore errors if it already exists)

def create_actor(mesh, texture, anims, props_):
	actor = ET.Element('actor', version='1')
	ET.SubElement(actor, 'castshadow')
	group = ET.SubElement(actor, 'group')
	variant = ET.SubElement(group, 'variant', frequency='100', name='Base')
	ET.SubElement(variant, 'mesh').text = mesh+'.pmd'
	ET.SubElement(variant, 'texture').text = texture+'.dds'

	animations = ET.SubElement(variant, 'animations')
	for name, file in anims:
		ET.SubElement(animations, 'animation', file=file+'.psa', name=name, speed='100')
	
	props = ET.SubElement(variant, 'props')
	for name, file in props_:
		ET.SubElement(props, 'prop', actor=file+'.xml', attachpoint=name)

	return ET.tostring(actor)
	
def create_actor_static(mesh, texture):
	actor = ET.Element('actor', version='1')
	ET.SubElement(actor, 'castshadow')
	group = ET.SubElement(actor, 'group')
	variant = ET.SubElement(group, 'variant', frequency='100', name='Base')
	ET.SubElement(variant, 'mesh').text = mesh+'.pmd'
	ET.SubElement(variant, 'texture').text = texture+'.dds'
	return ET.tostring(actor)

################################

# Error handling

if False:
	convert_dae_to_pmd('This is not well-formed XML', expected_status=-2)
	convert_dae_to_pmd('<html>This is not COLLADA</html>', expected_status=-2)
	convert_dae_to_pmd('<COLLADA>This is still not valid COLLADA</COLLADA>', expected_status=-2)

# Do some real conversions, so the output can be tested in the Actor Viewer

test_data = binaries + '/data/tests/collada'
test_mod = binaries + '/data/mods/_test.collada'

clean_dir(test_mod + '/art/meshes')
clean_dir(test_mod + '/art/actors')
clean_dir(test_mod + '/art/animation')

#for test_file in ['cube', 'jav2', 'jav2b', 'teapot_basic', 'teapot_skin', 'plane_skin', 'dude_skin', 'mergenonbone', 'densemesh']:
#for test_file in ['teapot_basic', 'jav2b', 'jav2d']:
for test_file in ['xsitest3c','xsitest3e','jav2d','jav2d2']:
#for test_file in ['xsitest3']:
#for test_file in []:
	print "* Converting PMD %s" % (test_file)

	input_filename = '%s/%s.dae' % (test_data, test_file)
	output_filename = '%s/art/meshes/%s.pmd' % (test_mod, test_file)
	
	input = open(input_filename).read()
	output = convert_dae_to_pmd(input)
	open(output_filename, 'wb').write(output)

	xml = create_actor(test_file, 'male', [('Idle','dudeidle'),('Corpse','dudecorpse'),('attack1',test_file),('attack2','jav2d')], [('helmet','teapot_basic_static')])
	open('%s/art/actors/%s.xml' % (test_mod, test_file), 'w').write(xml)

	xml = create_actor_static(test_file, 'male')
	open('%s/art/actors/%s_static.xml' % (test_mod, test_file), 'w').write(xml)

#for test_file in ['jav2','jav2b', 'jav2d']:
for test_file in ['xsitest3c','xsitest3e','jav2d','jav2d2']:
#for test_file in []:
	print "* Converting PSA %s" % (test_file)

	input_filename = '%s/%s.dae' % (test_data, test_file)
	output_filename = '%s/art/animation/%s.psa' % (test_mod, test_file)

	input = open(input_filename).read()
	output = convert_dae_to_psa(input)
	open(output_filename, 'wb').write(output)

