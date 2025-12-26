#!/usr/bin/env python3

"""Ninja build configurator for window library"""

import sys
import os

sys.path.insert(0, os.path.join('build', 'ninja'))

import generator

dependlibs = ['resource', 'vector', 'window', 'network', 'task', 'blake3', 'foundation']

libpaths = []
includepaths = []
if "VK_SDK_PATH" in os.environ:
    includepaths += [os.path.expandvars(os.path.join('$VK_SDK_PATH', 'Include'))]
    libpaths += [os.path.expandvars(os.path.join('$VK_SDK_PATH', 'Lib'))]

generator = generator.Generator(project='render', dependlibs=dependlibs, includepaths=includepaths, libpaths=libpaths,
                                variables=[('bundleidentifier', 'com.maniccoder.render.$(binname)')])
target = generator.target
writer = generator.writer
toolchain = generator.toolchain

render_lib = generator.lib(module='render', sources=[
    'backend.c', 'buffer.c', 'compile.c', 'event.c', 'import.c', 'pipeline.c', 'projection.c', 'render.c',
    'shader.c', 'target.c', 'version.c',
    os.path.join('directx12', 'backend.c'),
    os.path.join('metal', 'backend.m'), os.path.join('metal', 'backend.c'),
    os.path.join('vulkan', 'backend.c'),
    os.path.join('null', 'backend.c')
])

extralibs = []
gfxlibs = []
gfxframeworks = []
if target.is_macos():
    gfxframeworks = ['Metal', 'QuartzCore', 'CoreGraphics', 'Carbon']
elif target.is_ios():
    gfxframeworks = ['Metal', 'QuartzCore', 'OpenGLES']
if target.is_windows():
    gfxlibs = ['vulkan-1', 'gdi32', 'iphlpapi', 'ws2_32']
    extralibs = ['iphlpapi', 'ws2_32']
if target.is_linux():
    gfxlibs = ['vulkan', 'Xxf86vm', 'Xext', 'X11', 'GL']

dependlibs = ['render'] + dependlibs
linklibs = gfxlibs + extralibs

if not target.is_ios() and not target.is_android() and not target.is_tizen():
    configs = [config for config in toolchain.configs if config not in [
        'profile', 'deploy']]
    if not configs == []:
        generator.bin('renderimport', ['main.c'], 'renderimport', basepath='tools', implicit_deps=[
                      render_lib], dependlibs=dependlibs, libs=linklibs, frameworks=gfxframeworks, configs=configs)
        generator.bin('rendercompile', ['main.c'], 'rendercompile', basepath='tools', implicit_deps=[
                      render_lib], dependlibs=dependlibs, libs=linklibs, frameworks=gfxframeworks, configs=configs)

includepaths = generator.test_includepaths()

dependlibs = ['test'] + dependlibs

test_cases = [
    'render'
]
if toolchain.is_monolithic() or target.is_ios() or target.is_android() or target.is_tizen():
    # Build one fat binary with all test cases
    test_resources = []
    test_extrasources = []
    test_cases += ['all']
    if target.is_ios():
        test_resources = [os.path.join('all', 'ios', item) for item in [
            'test-all.plist', 'Images.xcassets', 'test-all.xib']]
        test_extrasources = [os.path.join('all', 'ios', 'viewcontroller.m')]
    elif target.is_android():
        test_resources = [os.path.join('all', 'android', item) for item in [
            'AndroidManifest.xml', os.path.join(
                'layout', 'main.xml'), os.path.join('values', 'strings.xml'),
            os.path.join('drawable-ldpi', 'icon.png'), os.path.join('drawable-mdpi',
                                                                    'icon.png'), os.path.join('drawable-hdpi', 'icon.png'),
            os.path.join('drawable-xhdpi', 'icon.png'), os.path.join('drawable-xxhdpi',
                                                                     'icon.png'), os.path.join('drawable-xxxhdpi', 'icon.png')
        ]]
        test_extrasources = [os.path.join('all', 'android', 'java', 'com', 'maniccoder', 'render', 'test', item) for item in [
            'TestActivity.java'
        ]]
    elif target.is_tizen():
        test_resources = [os.path.join('all', 'tizen', item) for item in [
            'tizen-manifest.xml', os.path.join('res', 'tizenapp.png')
        ]]
    if target.is_macos() or target.is_ios() or target.is_android() or target.is_tizen():
        generator.app(module='', sources=[os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname='test-all-render',
                      basepath='test', implicit_deps=[render_lib], libs=linklibs, frameworks=gfxframeworks, resources=test_resources, includepaths=includepaths)
    else:
        generator.bin(module='', sources=[os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname='test-all-render',
                      basepath='test', implicit_deps=[render_lib], libs=linklibs, frameworks=gfxframeworks, resources=test_resources, includepaths=includepaths)
else:
    # Build one binary per test case
    if not generator.is_subninja():
        generator.bin(module='all', sources=['main.c'], binname='test-all', basepath='test',
                      implicit_deps=[render_lib], libs=['render'] + dependlibs + extralibs, includepaths=includepaths)
    for test in test_cases:
        if target.is_macos():
            test_resources = [os.path.join('macos', item) for item in [
                'test-' + test + '.plist', 'test-' + test + '.entitlements', 'Images.xcassets', 'test-' + test + '.xib']]
            generator.app(module=test, sources=['main.c'], binname='test-' + test, basepath='test', implicit_deps=[
                          render_lib], dependlibs=dependlibs, libs=linklibs, frameworks=gfxframeworks, resources=test_resources, includepaths=includepaths)
        else:
            generator.bin(module=test, sources=['main.c'], binname='test-' + test, basepath='test', implicit_deps=[
                          render_lib], dependlibs=dependlibs, libs=linklibs, frameworks=gfxframeworks, includepaths=includepaths)
