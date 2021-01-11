#! /usr/bin/env python
import sys
import os
import starsplatter

filename = sys.argv[1]

mycam= starsplatter.Camera((-5.0,-9.0,3.0),
                           (0.0,0.0,0.0),
                           (0.0,0.0,1.0),
                           35.0,-1.0,-50.0)
# I think disk1 and bulge1 are Milky Way, disk2 and bulge2 are Andromeda
disk1= starsplatter.StarBunch()
disk2= starsplatter.StarBunch()
bulge1= starsplatter.StarBunch()
bulge2= starsplatter.StarBunch()

infile= open(filename,"r")
outList= starsplatter.load_dubinski(infile,[disk1, disk2, bulge1, bulge2])
infile.close()

myren= starsplatter.StarSplatter()
myren.set_image_dims(640, 480)
myren.set_camera(mycam)
myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_AUTO )
myren.set_exposure_scale(0.5)
myren.set_debug(1)

disk1.set_bunch_color((0.8, 0.5, 1.0, 1.0))
disk1.set_density(0.00002)
disk1.set_exp_constant(1000.0)
disk1.dump(sys.stderr)

disk2.set_bunch_color((0.5, 0.8, 0.0, 1.0))
disk2.set_density(0.00002)
disk2.set_exp_constant(1000.0)

bulge1.set_bunch_color((0.8, 0.7, 0.5, 1.0))
bulge1.set_density(0.000006)
bulge1.set_exp_constant(1000.0)

bulge2.set_bunch_color((0.8, 0.7, 0.5, 1.0))
bulge2.set_density(0.000006)
bulge2.set_exp_constant(1000.0)

for sb in [disk1, disk2, bulge1, bulge2]:
    myren.add_stars(sb)
img= myren.render()
black= starsplatter.rgbImage(img.xsize(),img.ysize())
black.clear((0.0,0.0,0.0,1.0))
img.add_under(black)
img.save("test.png","png")
print("Wrote test.png")


