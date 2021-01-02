#! /usr/bin/env python
import sys
import os
import starsplatter

def mkBunch( dx ):
    gp= starsplatter.StarBunch()
    gp.set_nstars(4)
    gp.set_coords(0,(0.0+dx,0.0,0.0))
    gp.set_coords(1,(5.0+dx,0.0,0.0))
    gp.set_coords(2,(0.0+dx,5.0,0.0))
    gp.set_coords(3,(0.0+dx,0.0,5.0))
    gp.set_bunch_color((0.1*dx,1.0-0.1*dx,1.0,1.0))
    return gp

mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)

myren= starsplatter.StarSplatter()
myren2= starsplatter.StarSplatter()
for i in xrange(0,5):
    sb= mkBunch(0.5*i)
    #sbList.append(sb)
    sb.set_density(0.03)
    sb.set_scale_length(0.1)
    print "%d..."%i,
    myren.add_stars(sb)
    if i%2 == 0: myren2.add_stars(sb)
    print "set."

myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )

myren.set_image_dims(640,480)
myren.set_camera(mycam)

img= myren.render()
img.save("test.png","png")
print "wrote test.png"
myren.clear_stars()

myren2.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )

myren2.set_image_dims(480,640)
myren2.set_camera(mycam)

img= myren2.render()
img.save("test2.png","png")
print "wrote test2.png"
myren2.clear_stars()

