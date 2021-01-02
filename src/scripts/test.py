#! /usr/bin/env python
import sys
import os
import starsplatter

mycam= starsplatter.Camera((0.0,0.0,3.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           30.0,-1.0,-5.0)
gas= starsplatter.StarBunch()
stars= starsplatter.StarBunch()
dark= starsplatter.StarBunch()

myren= starsplatter.StarSplatter()
infile= open("../data/test.tipsy","r")
starsplatter.load_tipsy_box_ascii(infile,gas,stars,dark)
infile.close()

gas.set_bunch_color((1.0,0.0,0.0,1.0))
gas.set_density(1.0)
gas.set_scale_length(0.1)
stars.set_bunch_color((0.0,1.0,0.0,1.0))
stars.set_density(1.0)
stars.set_scale_length(0.1)
dark.set_bunch_color((0.0,0.0,1.0,1.0))
dark.set_density(0.1)
dark.set_scale_length(0.1)
gas.dump(sys.stderr)
myren.set_image_dims(640,480)
myren.set_camera(mycam)
myren.set_debug(True)
myren.set_exposure_type(starsplatter.StarSplatter.ET_LOG_AUTO)
for sb in [gas,stars,dark]:
    myren.add_stars(sb)
img= myren.render()
black= starsplatter.rgbImage(img.xsize(),img.ysize())
black.clear((0.0,0.0,0.0,1.0))
img.add_under(black)
img.save("test.png","png")
print "wrote test.png"
