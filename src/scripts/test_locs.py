#! /usr/bin/env python
import sys
import os
import starsplatter

mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)

group1= starsplatter.StarBunch()
group1.set_nstars(4)
group1.set_coords(0,(0.0,0.0,0.0))
group1.set_coords(1,(5.0,0.0,0.0))
group1.set_coords(2,(0.0,5.0,0.0))
group1.set_coords(3,(0.0,0.0,5.0))
group1.set_bunch_color((1.0,1.0,1.0,1.0))

group2= starsplatter.StarBunch()
group2.set_nstars(1)
group2.set_coords(0,(-5.0,0.0,0.0))
group2.set_bunch_color((1.0,0.0,0.0,1.0))

group3= starsplatter.StarBunch()
group3.set_nstars(1)
group3.set_coords(0,(0.0,-5.0,0.0))
group3.set_bunch_color((0.0,1.0,0.0,1.0))

group4= starsplatter.StarBunch()
group4.set_nstars(1)
group4.set_coords(0,(0.0,0.0,-5.0))
group4.set_bunch_color((0.0,0.0,1.0,1.0))

myren= starsplatter.StarSplatter()
myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_AUTO )

myren.set_image_dims(640,480)
myren.set_camera(mycam)
myren.set_debug(1)

for sb in [ group1, group2, group3, group4 ]:
    sb.set_density(0.03)
    sb.set_scale_length(0.1)
    myren.add_stars(sb)

img= myren.render()
img.save("test.png","png")
print "wrote test.png"
