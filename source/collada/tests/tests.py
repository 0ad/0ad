from ctypes import *
import sys
import os
import xml.etree.ElementTree as ET

binaries = '../../../binaries'

dll_filename = {
	'posix': './libCollada_dbg.so',
	'nt': 'Collada.dll',
}[os.name]

library = cdll.LoadLibrary('%s/system/%s' % (binaries, dll_filename))

def log(severity, message):
	print '[%s] %s' % (('INFO', 'WARNING', 'ERROR')[severity], message)

clog = CFUNCTYPE(None, c_int, c_char_p)(log)
	# (the CFUNCTYPE must not be GC'd, so try to keep a reference)
library.set_logger(clog)

def convert_dae_to_pmd(filename):
	output = []
	def cb(str, len):
		output.append(string_at(str, len))

	cbtype = CFUNCTYPE(None, POINTER(c_char), c_uint)
	status = library.convert_dae_to_pmd(filename, cbtype(cb))
	assert(status == 0)
	return ''.join(output)

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
	
################################

test_data = binaries + '/data/tests/collada'
test_mod = binaries + '/data/mods/_test.collada'

clean_dir(test_mod + '/art/meshes')
clean_dir(test_mod + '/art/actors')

for test_file in ['cube', 'jav2']:
	input_filename = '%s/%s.dae' % (test_data, test_file)
	output_filename = '%s/art/meshes/%s.pmd' % (test_mod, test_file)
	
	input = open(input_filename).read()
	output = convert_dae_to_pmd(input)
	open(output_filename, 'wb').write(output)

	actor = ET.Element('actor', version='1')
	group = ET.SubElement(actor, 'group')
	variant = ET.SubElement(group, 'variant', frequency='100', name='Base')
	ET.SubElement(variant, 'mesh').text = test_file+'.pmd'
	ET.SubElement(variant, 'texture').text = 'male.dds'
	
	open('%s/art/actors/%s.xml' % (test_mod, test_file), 'w').write(ET.tostring(actor))
