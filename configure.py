#!/usr/bin/env python

"""Ninja build configurator for window library"""

import sys
import os

sys.path.insert(0, os.path.join('build', 'ninja'))

import generator

dependlibs = ['resource', 'vector', 'window', 'network', 'foundation']

generator = generator.Generator(project = 'render', dependlibs = dependlibs, variables = [('bundleidentifier', 'com.rampantpixels.render.$(binname)')])
target = generator.target
writer = generator.writer
toolchain = generator.toolchain

render_lib = generator.lib(module = 'render', sources = [
  'backend.c', 'buffer.c', 'command.c', 'context.c', 'compile.c', 'drawable.c', 'event.c', 'indexbuffer.c', 'import.c',
  'parameter.c', 'pipeline.c', 'program.c', 'projection.c', 'render.c', 'shader.c', 'state.c', 'sort.c', 'target.c',
  'version.c', 'vertexbuffer.c',
  os.path.join('gl4', 'backend.c'), os.path.join('gl4', 'backend.m'), os.path.join('gl4', 'glprocs.c'),
  os.path.join('gl2', 'backend.c'),
  os.path.join('gles2', 'backend.c'),
  os.path.join('null', 'backend.c')
])

gllibs = []
glframeworks = []
if target.is_macos():
  glframeworks = ['OpenGL']
elif target.is_ios():
  glframeworks = ['QuartzCore', 'OpenGLES']
if target.is_windows():
  gllibs = ['opengl32', 'gdi32', 'iphlpapi', 'ws2_32']
if target.is_linux():
  gllibs = ['GL', 'Xxf86vm', 'Xext', 'X11']

linklibs = ['render'] + gllibs

if not target.is_ios() and not target.is_android() and not target.is_tizen():
  configs = [config for config in toolchain.configs if config not in ['profile', 'deploy']]
  if not configs == []:
    generator.bin('renderimport', ['main.c'], 'renderimport', basepath = 'tools', implicit_deps = [render_lib], dependlibs = dependlibs, libs = linklibs, frameworks = glframeworks, configs = configs)
    generator.bin('rendercompile', ['main.c'], 'rendercompile', basepath = 'tools', implicit_deps = [render_lib], dependlibs = dependlibs, libs = linklibs, frameworks = glframeworks, configs = configs)

#No test cases if we're a submodule
if generator.is_subninja():
  sys.exit()

includepaths = generator.test_includepaths()

linklibs = ['test'] + linklibs

test_cases = [
  'render'
]
if toolchain.is_monolithic() or target.is_ios() or target.is_android() or target.is_tizen() or target.is_pnacl():
  #Build one fat binary with all test cases
  test_resources = []
  test_extrasources = []
  test_cases += ['all']
  if target.is_ios():
    test_resources = [os.path.join('all', 'ios', item) for item in ['test-all.plist', 'Images.xcassets', 'test-all.xib']]
    test_extrasources = [os.path.join('all', 'ios', 'viewcontroller.m')]
  elif target.is_android():
    test_resources = [os.path.join('all', 'android', item) for item in [
      'AndroidManifest.xml', os.path.join('layout', 'main.xml'), os.path.join('values', 'strings.xml'),
      os.path.join('drawable-ldpi', 'icon.png'), os.path.join('drawable-mdpi', 'icon.png'), os.path.join('drawable-hdpi', 'icon.png'),
      os.path.join('drawable-xhdpi', 'icon.png'), os.path.join('drawable-xxhdpi', 'icon.png'), os.path.join('drawable-xxxhdpi', 'icon.png')
    ]]
    test_extrasources = [os.path.join('all', 'android', 'java', 'com', 'rampantpixels', 'render', 'test', item) for item in [
      'TestActivity.java'
    ]]
  elif target.is_tizen():
    test_resources = [os.path.join('all', 'tizen', item) for item in [
      'tizen-manifest.xml', os.path.join('res', 'tizenapp.png')
    ]]
  if target.is_macos() or target.is_ios() or target.is_android() or target.is_tizen():
    generator.app(module = '', sources = [os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname = 'test-all', basepath = 'test', implicit_deps = [render_lib], libs = linklibs, frameworks = glframeworks, resources = test_resources, includepaths = includepaths)
  else:
    generator.bin(module = '', sources = [os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname = 'test-all', basepath = 'test', implicit_deps = [render_lib], libs = linklibs, frameworks = glframeworks, resources = test_resources, includepaths = includepaths)
else:
  #Build one binary per test case
  generator.bin(module = 'all', sources = ['main.c'], binname = 'test-all', basepath = 'test', implicit_deps = [render_lib], libs = ['render'] + dependlibs, includepaths = includepaths)
  for test in test_cases:
    if target.is_macos():
      test_resources = [os.path.join('osx', item) for item in ['test-' + test + '.plist', 'Images.xcassets', 'test-' + test + '.xib']]
      generator.app(module = test, sources = ['main.c' ], binname = 'test-' + test, basepath = 'test', implicit_deps = [render_lib], libs = linklibs, frameworks = glframeworks, resources = test_resources, includepaths = includepaths)
    else:
      generator.bin(module = test, sources = ['main.c' ], binname = 'test-' + test, basepath = 'test', implicit_deps = [render_lib], libs = linklibs, frameworks = glframeworks, includepaths = includepaths)
