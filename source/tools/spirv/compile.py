#!/usr/bin/env python3
# -*- mode: python-mode; python-indent-offset: 4; -*-
#
# Copyright (C) 2023 Wildfire Games.
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
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import argparse
import datetime
import hashlib
import itertools
import json
import os
import subprocess
import sys
import time
import yaml

import xml.etree.ElementTree as ET


def execute(command):
    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = process.communicate()
    except:
        sys.stderr.write('Failed to run command: {}\n'.format(' '.join(command)))
        raise
    return process.returncode, out, err

def calculate_hash(path):
    assert os.path.isfile(path)
    with open(path, 'rb') as handle:
        return hashlib.sha1(handle.read()).hexdigest()

def compare_spirv(path1, path2):
    with open(path1, 'rb') as handle:
        spirv1 = handle.read()
    with open(path2, 'rb') as handle:
        spirv2 = handle.read()
    return spirv1 == spirv2

def resolve_if(defines, expression):
    for item in expression.strip().split('||'):
        item = item.strip()
        assert len(item) > 1
        name = item
        invert = False
        if name[0] == '!':
            invert = True
            name = item[1:]
            assert item[1].isalpha()
        else:
            assert item[0].isalpha()
        found_define = False
        for define in defines:
            if define['name'] == name:
                assert define['value'] == 'UNDEFINED' or define['value'] == '0' or define['value'] == '1'
                if invert:
                    if define['value'] != '1':
                        return True
                    found_define = True
                else:
                    if define['value'] == '1':
                        return True
        if invert and not found_define:
            return True
    return False

def compile_and_reflect(input_mod_path, output_mod_path, dependencies, stage, path, out_path, defines):
    keep_debug = False
    input_path = os.path.normpath(path)
    output_path = os.path.normpath(out_path)
    command = [
        'glslc', '-x', 'glsl', '--target-env=vulkan1.1', '-std=450core',
        '-I', os.path.join(input_mod_path, 'shaders', 'glsl'),
    ]
    for dependency in dependencies:
        if dependency != input_mod_path:
            command += ['-I', os.path.join(dependency, 'shaders', 'glsl')]
    command += [
        '-fshader-stage=' + stage, '-O', input_path,
    ]
    use_descriptor_indexing = False
    for define in defines:
        if define['value'] == 'UNDEFINED':
            continue
        assert ' ' not in define['value']
        command.append('-D{}={}'.format(define['name'], define['value']))
        if define['name'] == 'USE_DESCRIPTOR_INDEXING':
            use_descriptor_indexing = True
    command.append('-D{}={}'.format('USE_SPIRV', '1'))
    command.append('-DSTAGE_{}={}'.format(stage.upper(), '1'))
    command += ['-o', output_path]
    # Compile the shader with debug information to see names in reflection.
    ret, out, err = execute(command + ['-g'])
    if ret:
        sys.stderr.write('Command returned {}:\nCommand: {}\nInput path: {}\nOutput path: {}\nError: {}\n'.format(
            ret, ' '.join(command), input_path, output_path, err))
        preprocessor_output_path = os.path.abspath(os.path.join(os.path.dirname(__file__), 'preprocessed_file.glsl'))
        execute(command[:-2] + ['-g', '-E', '-o', preprocessor_output_path])
        raise ValueError(err)
    ret, out, err = execute(['spirv-reflect', '-y','-v', '1', output_path])
    if ret:
        sys.stderr.write('Command returned {}:\nCommand: {}\nInput path: {}\nOutput path: {}\nError: {}\n'.format(
            ret, ' '.join(command), input_path, output_path, err))
        raise ValueError(err)
    # Reflect the result SPIRV.
    data = yaml.safe_load(out)
    module = data['module']
    interface_variables = []
    if 'all_interface_variables' in data and data['all_interface_variables']:
        interface_variables = data['all_interface_variables']
    push_constants = []
    vertex_attributes = []
    if 'push_constants' in module and module['push_constants']:
        assert len(module['push_constants']) == 1
        def add_push_constants(node, push_constants):
            if ('members' in node) and node['members']:
                for member in node['members']:
                    add_push_constants(member, push_constants)
            else:
                assert node['absolute_offset'] + node['size'] <= 128
                push_constants.append({
                    'name': node['name'],
                    'offset': node['absolute_offset'],
                    'size': node['size'],
                })
        assert module['push_constants'][0]['type_description']['type_name'] == 'DrawUniforms'
        assert module['push_constants'][0]['size'] <= 128
        add_push_constants(module['push_constants'][0], push_constants)
    descriptor_sets = []
    if 'descriptor_sets' in module and module['descriptor_sets']:
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7
        for descriptor_set in module['descriptor_sets']:
            UNIFORM_SET = 1 if use_descriptor_indexing else 0
            STORAGE_SET = 2
            bindings = []
            if descriptor_set['set'] == UNIFORM_SET:
                assert descriptor_set['binding_count'] > 0
                for binding in descriptor_set['bindings']:
                    assert binding['set'] == UNIFORM_SET
                    block = binding['block']
                    members = []
                    for member in block['members']:
                        members.append({
                            'name': member['name'],
                            'offset': member['absolute_offset'],
                            'size': member['size'],
                        })
                    bindings.append({
                        'binding': binding['binding'],
                        'type': 'uniform',
                        'size': block['size'],
                        'members': members
                    })
                binding = descriptor_set['bindings'][0]
                assert binding['descriptor_type'] == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            elif descriptor_set['set'] == STORAGE_SET:
                assert descriptor_set['binding_count'] > 0
                for binding in descriptor_set['bindings']:
                    is_storage_image = binding['descriptor_type'] == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                    is_storage_buffer = binding['descriptor_type'] == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                    assert is_storage_image or is_storage_buffer
                    assert binding['descriptor_type'] == descriptor_set['bindings'][0]['descriptor_type']
                    assert binding['image']['arrayed'] == 0
                    assert binding['image']['ms'] == 0
                    bindingType = 'storageImage'
                    if is_storage_buffer:
                        bindingType = 'storageBuffer'
                    bindings.append({
                        'binding': binding['binding'],
                        'type': bindingType,
                        'name': binding['name'],
                    })
            else:
                if use_descriptor_indexing:
                    if descriptor_set['set'] == 0:
                        assert descriptor_set['binding_count'] >= 1
                        for binding in descriptor_set['bindings']:
                            assert binding['descriptor_type'] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                            assert binding['array']['dims'][0] == 16384
                            if binding['binding'] == 0:
                                assert binding['name'] == 'textures2D'
                            elif binding['binding'] == 1:
                                assert binding['name'] == 'texturesCube'
                            elif binding['binding'] == 2:
                                assert binding['name'] == 'texturesShadow'
                    else:
                        assert False
                else:
                    assert descriptor_set['binding_count'] > 0
                    for binding in descriptor_set['bindings']:
                        assert binding['descriptor_type'] == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        assert binding['image']['sampled'] == 1
                        assert binding['image']['arrayed'] == 0
                        assert binding['image']['ms'] == 0
                        sampler_type = 'sampler{}D'.format(binding['image']['dim'] + 1)
                        if binding['image']['dim'] == 3:
                            sampler_type = 'samplerCube'
                        bindings.append({
                            'binding': binding['binding'],
                            'type': sampler_type,
                            'name': binding['name'],
                        })
            descriptor_sets.append({
                'set': descriptor_set['set'],
                'bindings': bindings,
            })
    if stage == 'vertex':
        for variable in interface_variables:
            if variable['storage_class'] == 1:
                # Input.
                vertex_attributes.append({
                    'name': variable['name'],
                    'location': variable['location'],
                })
    # Compile the final version without debug information.
    if not keep_debug:
        ret, out, err = execute(command)
        if ret:
            sys.stderr.write('Command returned {}:\nCommand: {}\nInput path: {}\nOutput path: {}\nError: {}\n'.format(
                ret, ' '.join(command), input_path, output_path, err))
            raise ValueError(err)
    return {
        'push_constants': push_constants,
        'vertex_attributes': vertex_attributes,
        'descriptor_sets': descriptor_sets,
    }


def output_xml_tree(tree, path):
    ''' We use a simple custom printer to have the same output for all platforms.'''
    with open(path, 'wt') as handle:
        handle.write('<?xml version="1.0" encoding="utf-8"?>\n')
        handle.write('<!-- DO NOT EDIT: GENERATED BY SCRIPT {} -->\n'.format(os.path.basename(__file__)))
        def output_xml_node(node, handle, depth):
            indent = '\t' * depth
            attributes = ''
            for attribute_name in sorted(node.attrib.keys()):
                attributes += ' {}="{}"'.format(attribute_name, node.attrib[attribute_name])
            if len(node) > 0:
                handle.write('{}<{}{}>\n'.format(indent, node.tag, attributes))
                for child in node:
                    output_xml_node(child, handle, depth + 1)
                handle.write('{}</{}>\n'.format(indent, node.tag))
            else:
                handle.write('{}<{}{}/>\n'.format(indent, node.tag, attributes))
        output_xml_node(tree.getroot(), handle, 0)


def build(rules, input_mod_path, output_mod_path, dependencies, program_name):
    sys.stdout.write('Program "{}"\n'.format(program_name))
    if rules and program_name not in rules:
        sys.stdout.write('  Skip.\n')
        return
    sys.stdout.write('  Building.\n')

    rebuild = False

    defines = []
    program_defines = []
    shaders = []

    tree = ET.parse(os.path.join(input_mod_path, 'shaders', 'glsl', program_name + '.xml'))
    root = tree.getroot()
    for element in root:
        element_tag = element.tag
        if element_tag == 'defines':
            for child in element:
                values = []
                for value in child:
                    values.append({
                        'name': child.attrib['name'],
                        'value': value.text,
                    })
                defines.append(values)
        elif element_tag == 'define':
            program_defines.append({'name': element.attrib['name'], 'value': element.attrib['value']})
        elif element_tag == 'vertex':
            streams = []
            for shader_child in element:
                assert shader_child.tag == 'stream'
                streams.append({
                    'name': shader_child.attrib['name'],
                    'attribute': shader_child.attrib['attribute'],
                })
                if 'if' in shader_child.attrib:
                    streams[-1]['if'] = shader_child.attrib['if']
            shaders.append({
                'type': 'vertex',
                'file': element.attrib['file'],
                'streams': streams,
            })
        elif element_tag == 'fragment':
            shaders.append({
                'type': 'fragment',
                'file': element.attrib['file'],
            })
        elif element_tag == 'compute':
            shaders.append({
                'type': 'compute',
                'file': element.attrib['file'],
            })
        else:
            raise ValueError('Unsupported element tag: "{}"'.format(element_tag))

    stage_extension = {
        'vertex': '.vs',
        'fragment': '.fs',
        'geometry': '.gs',
        'compute': '.cs',
    }

    output_spirv_mod_path = os.path.join(output_mod_path, 'shaders', 'spirv')
    if not os.path.isdir(output_spirv_mod_path):
        os.mkdir(output_spirv_mod_path)

    root = ET.Element('programs')

    if 'combinations' in rules[program_name]:
        combinations = rules[program_name]['combinations']
    else:
        combinations = list(itertools.product(*defines))

    hashed_cache = {}

    for index, combination in enumerate(combinations):
        assert index < 10000
        program_path = 'spirv/' + program_name + ('_%04d' % index) + '.xml'

        programs_element = ET.SubElement(root, 'program')
        programs_element.set('type', 'spirv')
        programs_element.set('file', program_path)

        defines_element = ET.SubElement(programs_element, 'defines')
        for define in combination:
            if define['value'] == 'UNDEFINED':
                continue
            define_element = ET.SubElement(defines_element, 'define')
            define_element.set('name', define['name'])
            define_element.set('value', define['value'])

        if not rebuild and os.path.isfile(os.path.join(output_mod_path, 'shaders', program_path)):
            continue

        program_root = ET.Element('program')
        program_root.set('type', 'spirv')
        for shader in shaders:
            extension = stage_extension[shader['type']]
            file_name = program_name + ('_%04d' % index) + extension + '.spv'
            output_spirv_path = os.path.join(output_spirv_mod_path, file_name)

            input_glsl_path = os.path.join(input_mod_path, 'shaders', shader['file'])
            # Some shader programs might use vs and fs shaders from different mods.
            if not os.path.isfile(input_glsl_path):
                input_glsl_path = None
                for dependency in dependencies:
                    fallback_input_path = os.path.join(dependency, 'shaders', shader['file'])
                    if os.path.isfile(fallback_input_path):
                        input_glsl_path = fallback_input_path
                        break
            assert input_glsl_path is not None

            reflection = compile_and_reflect(
                input_mod_path,
                output_mod_path,
                dependencies,
                shader['type'],
                input_glsl_path,
                output_spirv_path,
                combination + program_defines)

            spirv_hash = calculate_hash(output_spirv_path)
            if spirv_hash not in hashed_cache:
                hashed_cache[spirv_hash] = [file_name]
            else:
                found_candidate = False
                for candidate_name in hashed_cache[spirv_hash]:
                    candidate_path = os.path.join(output_spirv_mod_path, candidate_name)
                    if compare_spirv(output_spirv_path, candidate_path):
                        found_candidate = True
                        file_name = candidate_name
                        break
                if found_candidate:
                    os.remove(output_spirv_path)
                else:
                    hashed_cache[spirv_hash].append(file_name)

            shader_element = ET.SubElement(program_root, shader['type'])
            shader_element.set('file', 'spirv/' + file_name)
            if shader['type'] == 'vertex':
                for stream in shader['streams']:
                    if 'if' in stream and not resolve_if(combination, stream['if']):
                        continue

                    found_vertex_attribute = False
                    for vertex_attribute in reflection['vertex_attributes']:
                        if vertex_attribute['name'] == stream['attribute']:
                            found_vertex_attribute = True
                            break
                    if not found_vertex_attribute and stream['attribute'] == 'a_tangent':
                        continue
                    if not found_vertex_attribute:
                        sys.stderr.write('Vertex attribute not found: {}\n'.format(stream['attribute']))
                    assert found_vertex_attribute

                    stream_element = ET.SubElement(shader_element, 'stream')
                    stream_element.set('name', stream['name'])
                    stream_element.set('attribute', stream['attribute'])
                    for vertex_attribute in reflection['vertex_attributes']:
                        if vertex_attribute['name'] == stream['attribute']:
                            stream_element.set('location', vertex_attribute['location'])
                            break

            for push_constant in reflection['push_constants']:
                push_constant_element = ET.SubElement(shader_element, 'push_constant')
                push_constant_element.set('name', push_constant['name'])
                push_constant_element.set('size', push_constant['size'])
                push_constant_element.set('offset', push_constant['offset'])
            descriptor_sets_element = ET.SubElement(shader_element, 'descriptor_sets')
            for descriptor_set in reflection['descriptor_sets']:
                descriptor_set_element = ET.SubElement(descriptor_sets_element, 'descriptor_set')
                descriptor_set_element.set('set', descriptor_set['set'])
                for binding in descriptor_set['bindings']:
                    binding_element = ET.SubElement(descriptor_set_element, 'binding')
                    binding_element.set('type', binding['type'])
                    binding_element.set('binding', binding['binding'])
                    if binding['type'] == 'uniform':
                        binding_element.set('size', binding['size'])
                        for member in binding['members']:
                            member_element = ET.SubElement(binding_element, 'member')
                            member_element.set('name', member['name'])
                            member_element.set('size', member['size'])
                            member_element.set('offset', member['offset'])
                    elif binding['type'].startswith('sampler'):
                        binding_element.set('name', binding['name'])
                    elif binding['type'].startswith('storage'):
                        binding_element.set('name', binding['name'])
        program_tree = ET.ElementTree(program_root)
        output_xml_tree(program_tree, os.path.join(output_mod_path, 'shaders', program_path))


    tree = ET.ElementTree(root)
    output_xml_tree(tree, os.path.join(output_mod_path, 'shaders', 'spirv', program_name + '.xml'))


def run():
    parser = argparse.ArgumentParser()
    parser.add_argument('input_mod_path', help='a path to a directory with input mod with GLSL shaders like binaries/data/mods/public')
    parser.add_argument('rules_path', help='a path to JSON with rules')
    parser.add_argument('output_mod_path', help='a path to a directory with mod to store SPIR-V shaders like binaries/data/mods/spirv')
    parser.add_argument('-d', '--dependency', action='append', help='a path to a directory with a dependency mod (at least modmod should present as dependency)', required=True)
    parser.add_argument('-p', '--program_name', help='a shader program name (in case of presence the only program will be compiled)', default=None)
    args = parser.parse_args()

    if not os.path.isfile(args.rules_path):
        sys.stderr.write('Rules "{}" are not found\n'.format(args.rules_path))
        return
    with open(args.rules_path, 'rt') as handle:
        rules = json.load(handle)

    if not os.path.isdir(args.input_mod_path):
        sys.stderr.write('Input mod path "{}" is not a directory\n'.format(args.input_mod_path))
        return

    if not os.path.isdir(args.output_mod_path):
        sys.stderr.write('Output mod path "{}" is not a directory\n'.format(args.output_mod_path))
        return

    mod_shaders_path = os.path.join(args.input_mod_path, 'shaders', 'glsl')
    if not os.path.isdir(mod_shaders_path):
        sys.stderr.write('Directory "{}" was not found\n'.format(mod_shaders_path))
        return

    mod_name = os.path.basename(os.path.normpath(args.input_mod_path))
    sys.stdout.write('Building SPIRV for "{}"\n'.format(mod_name))
    if not args.program_name:
        for file_name in os.listdir(mod_shaders_path):
            name, ext = os.path.splitext(file_name)
            if ext.lower() == '.xml':
                build(rules, args.input_mod_path, args.output_mod_path, args.dependency, name)
    else:
        build(rules, args.input_mod_path, args.output_mod_path, args.dependency, args.program_name)

if __name__ == '__main__':
    run()

