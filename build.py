import os
import sys
joinp = os.path.join
sys.path.insert(0, 'whitgl')
sys.path.insert(0, joinp('whitgl', 'input'))
import build
import platform
import ninja_syntax


def main():
  target = 'nest'
  srcdir = 'src'
  inputdir = joinp('whitgl', 'input')
  builddir = 'build'
  targetdir = joinp(builddir, 'out')
  if build.plat == 'Darwin':
    packagedir = joinp(targetdir, 'Nest.app', 'Contents')
    executabledir = joinp(packagedir, 'MacOS')
    data_out = joinp(packagedir, 'Resources', 'data')
  else:
    executabledir = targetdir
    data_out = joinp(targetdir, 'data')
  objdir = joinp(builddir, 'obj')
  libdir = joinp(builddir, 'lib')
  data_in =  'data'
  buildfile = open(joinp('build','build.ninja'), 'w')
  n = ninja_syntax.Writer(buildfile)
  cflags, ldflags = build.flags(inputdir)
  cflags += ' -Iwhitgl/inc -Isrc -g -O2'
  n.variable('cflags', cflags)
  n.variable('ldflags', ldflags)
  n.variable('scripts_dir', joinp('whitgl','scripts'))
  build.rules(n)
  if build.plat == 'Windows':
    n.rule('windres', command='windres $in -O coff -o $out', description='windres $out')
  obj = build.walk_src(n, srcdir, objdir)
  if build.plat == 'Windows':
    obj += n.build(joinp(objdir, 'icon.res'), 'windres', joinp(data_in, 'win', 'Nest.rc'))
  whitgl = [joinp('whitgl','build','lib','whitgl.a')]
  targets = []
  targets += n.build(joinp(executabledir, target), 'link', obj+whitgl)
  n.newline()

  data = build.walk_data(n, data_in, data_out)

  targets += n.build('data', 'phony', data)
  n.newline()

  targets += build.copy_libs(n, inputdir, executabledir)

  if build.plat == 'Darwin':
    targets += n.build(joinp(packagedir, 'Info.plist'), 'cp', joinp(data_in, 'osx', 'Info.plist'))
    targets += n.build(joinp(packagedir, 'Resources', 'Nest.icns'), 'cp', joinp(data_in, 'osx', 'Nest.icns'))

  n.build('all', 'phony', targets)
  n.default('all')

if __name__ == '__main__':
  main()
