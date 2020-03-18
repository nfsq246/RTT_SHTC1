from building import *
Import('rtconfig')

src   = []
cwd   = GetCurrentDir()

src += Glob('sensor_sr_shtc1.c')
src += Glob('sensirion_i2c.c')
src += Glob('lib/shtc1.c')
src += Glob('lib/sensirion_common.c')

# add lsm6dsl include path.
path  = [cwd, cwd + '/lib']

# add src and include to group.
group = DefineGroup('shtc1', src, depend = ['PKG_USING_SHTC1'], CPPPATH = path)

Return('group')
