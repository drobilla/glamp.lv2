#!/usr/bin/env python
from waflib.extras import autowaf as autowaf
import waflib.Options as Options
import re

# Variables for 'waf dist'
APPNAME = 'glamp.lv2'
VERSION = '0.0.0'

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')
    opt.load('lv2')
    if Options.platform == 'win32':
        opt.load('compiler_cxx')
    autowaf.set_options(opt)

def configure(conf):
    conf.load('compiler_c')
    conf.load('lv2')
    if conf.env.DEST_OS == 'win32':
        conf.load('compiler_cxx')

    autowaf.configure(conf)
    autowaf.set_c99_mode(conf)
    autowaf.display_header('Glamp Configuration')

    conf.check_cc(header_name='GL/glu.h')

    autowaf.check_pkg(conf, 'lv2', atleast_version='1.6.0',
                      uselib_store='LV2')
    autowaf.check_pkg(conf, 'glfw3', atleast_version='3.3.0',
                      uselib_store='GLFW3')

    autowaf.display_msg(conf, 'LV2 bundle directory', conf.env.LV2DIR)
    print('')

def build(bld):
    bundle = 'glamp.lv2'

    # Make a pattern for shared objects without the 'lib' prefix
    module_pat = re.sub('^lib', '', bld.env.cshlib_PATTERN)
    module_ext = module_pat[module_pat.rfind('.'):]

    # Determine platform options
    ui_framework = []
    ui_libs      = []
    ui_lang      = 'c'
    if bld.env.DEST_OS == 'win32':
        ui_type = 'http://lv2plug.in/ns/extensions/ui#WindowsUI'
        ui_libs = ['opengl32', 'glu32', 'gdi32', 'user32']
        ui_lang = 'cxx'
    elif bld.env.DEST_OS == 'darwin':
        ui_type      = 'http://lv2plug.in/ns/extensions/ui#CocoaUI'
        ui_framework = ['Cocoa', 'OpenGL']
    else:
        ui_type = 'http://lv2plug.in/ns/extensions/ui#X11UI'
        ui_libs = ['X11', 'Xrandr', 'Xinerama', 'Xcursor', 'GL', 'GLU']

    # Build Turtle files by substitution
    for i in ['manifest.ttl', 'glamp.ttl']:
        bld(features     = 'subst',
            source       = i + '.in',
            target       = '%s/%s' % (bundle, i),
            install_path = '${LV2DIR}/%s' % bundle,
            LIB_EXT      = module_ext,
            UI_TYPE      = ui_type)

    # Build plugin library
    obj = bld(features     = 'c cshlib',
              source       = 'glamp.c',
              name         = 'glamp',
              target       = '%s/glamp' % bundle,
              install_path = '${LV2DIR}/%s' % bundle,
              use          = 'LV2',
              lib          = ['m'])
    obj.env.cshlib_PATTERN = module_pat

    # Build UI library
    obj = bld(features     = '%s %sshlib' % (ui_lang, ui_lang),
              source       = 'glamp_ui.c',
              name         = 'glamp_ui',
              target       = '%s/glamp_ui' % bundle,
              install_path = '${LV2DIR}/%s' % bundle,
              lib          = ui_libs + ['m', 'pthread', 'dl', 'X11'],
              framework    = ui_framework,
              use          = 'LV2 GLFW3')
    obj.env['%sshlib_PATTERN' % ui_lang] = module_pat
