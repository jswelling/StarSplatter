#! /usr/bin/env python
import sys
import os
from distutils.core import setup, Extension
from distutils.command.build import build
from distutils.command.install import install

class CustomBuild(build):
    def run(self):
        self.run_command('build_ext')
        build.run(self)


class CustomInstall(install):
    pass
#    def run(self):
#        self.run_command('build_ext')
#        self.do_egg_install()


srcFileList= [ "camera.cc", "geometry.cc", "rgbimage.cc",
               "starsplatter.cc", "ssplat_usr_modify.cc",
               "starbunch.cc", "utils.cc", "interpolate.cc", "splatpainter.cc",
               "gaussiansplatpainter.cc", "splinesplatpainter.cc",
               "circlesplatpainter.cc", "cball.cc",
               "starsplatter.i", "cball.i" ]

starsplatter_ext = Extension('_starsplatter', srcFileList,
                             include_dirs=['.'],
                             define_macros=[('NDEBUG','1'),
                                            ('AVOID_IMTOOLS',None),
                                            ('INCL_PNG',None)],
                             libraries=['png','m'],
                             swig_opts=['-c++', '-py3']
                             )


setup(name="starsplatter",
      version='2.1.2',
      cmdclass={'build': CustomBuild, 'install': CustomInstall},
      ext_modules=[starsplatter_ext],
      py_modules=['starsplatter'],
      description='A tool for rendering SPH simulation data',
      author='Joel Welling',
      author_email='welling@psc.edu',
      url='http://www.psc.edu/Packages/StarSplatter_Home'
      )
