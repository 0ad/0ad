from ctypes import *
import sys
import os

if len(sys.argv) != 3:
	print "Incorrect command-line syntax. Use"
	print "    " + sys.argv[0] + " input.dae output.pmd"
	sys.exit(-1)

input_filename, output_filename = sys.argv[1:]

if not os.path.exists(input_filename):
	print "Cannot find input file '%s'" % input_filename
	sys.exit(-1)

dll_filename = {
	'posix': './libCollada_dbg.so',
	'nt': 'Collada.dll',
}[os.name]

library = cdll.LoadLibrary(dll_filename)

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

input = open(input_filename).read()
output = convert_dae_to_pmd(input)
open(output_filename, 'wb').write(output)

