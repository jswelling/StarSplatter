#! /usr/bin/env python
import sys
import os
from distutils.core import setup, Extension

srcFileList= [ "camera.cc", "geometry.cc", "rgbimage.cc",
               "starsplatter.cc", "ssplat_usr_modify.cc",
               "starbunch.cc", "utils.cc", "interpolate.cc", "splatpainter.cc",
               "gaussiansplatpainter.cc", "splinesplatpainter.cc",
               "circlesplatpainter.cc", "cball.cc",
               "starsplatter.i", "cball.i" ]


setup(name="starsplatter",
      version='2.1.2',
      ext_modules=[Extension('_starsplatter',srcFileList,
                             include_dirs=['.'],
                             define_macros=[('NDEBUG','1'),
                                            ('AVOID_IMTOOLS',None),
                                            ('INCL_PNG',None)],
                             libraries=['png','m'],
                             swig_opts=['-c++'],
                             ),
                   ],
      py_modules=['starsplatter'],
      description='A tool for rendering SPH simulation data',
      author='Joel Welling',
      author_email='welling@psc.edu',
      url='http://www.psc.edu/Packages/StarSplatter_Home'
      )
