import os
import sys
import rtconfig
import subprocess
import platform
import uuid

if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = os.path.normpath(os.getcwd() + '/../../..')

# set RTT_ROOT
if not os.getenv("RTT_ROOT"): 
    RTT_ROOT="rt-thread"

sys.path = sys.path + [os.path.join(RTT_ROOT, 'tools')]
try:
    from building import *
except:
    print('Cannot found RT-Thread root directory, please check RTT_ROOT')
    print(RTT_ROOT)
    exit(-1)

TARGET = 'rtthread.' + rtconfig.TARGET_EXT

DefaultEnvironment(tools=[])
env = Environment(tools = ['mingw'],
	AS = rtconfig.AS, ASFLAGS = rtconfig.AFLAGS,
	CC = rtconfig.CC, CFLAGS = rtconfig.CFLAGS,
	CXX = rtconfig.CXX, CXXFLAGS = rtconfig.CFLAGS,
	AR = rtconfig.AR, ARFLAGS = '-rc',
	LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS)
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)

Export('RTT_ROOT')
Export('rtconfig')

SDK_ROOT = os.path.abspath('./')

if os.path.exists(SDK_ROOT + '/libraries'):
    libraries_path_prefix = SDK_ROOT + '/libraries'
else:
    libraries_path_prefix = os.path.dirname(SDK_ROOT) + '/libraries'

SDK_LIB = libraries_path_prefix
Export('SDK_LIB')

# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)

# set spawn
def ourspawn(sh, escape, cmd, args, e):
    filename = str(uuid.uuid4())
    newargs = ' '.join(args[1:])
    cmdline = cmd + " " + newargs
    if (len(cmdline) > 16 * 1024):
        f = open(filename, 'w')
        f.write(' '.join(args[1:]).replace('\\', '/'))
        f.close()
        # exec
        cmdline = cmd + " @" + filename
    proc = subprocess.Popen(cmdline, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell = False, env = e)
    data, err = proc.communicate()
    rv = proc.wait()
    def res_output(_output, _s):
        if len(_s):
            if isinstance(_s, str):
                _output(_s)
            elif isinstance(_s, bytes):
                _output(str(_s, 'UTF-8'))
            else:
                _output(str(_s))
    res_output(sys.stderr.write, err)
    res_output(sys.stdout.write, data)
    if os.path.isfile(filename):
        os.remove(filename)
    return rv

if platform.system() == 'Windows':
    env['SPAWN'] = ourspawn

nuvoton_library = 'nuc980'
rtconfig.BSP_LIBRARY_TYPE = nuvoton_library

# include libraries
objs.extend(SConscript(os.path.join(libraries_path_prefix, nuvoton_library, 'SConscript')))

# include nu_pkgs
objs.extend(SConscript(os.path.join(libraries_path_prefix, 'nu_packages', 'SConscript')))

# make a building
DoBuilding(TARGET, objs)
