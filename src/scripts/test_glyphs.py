#! /usr/bin/env python
import sys
import os
import starsplatter

mycam= starsplatter.Camera((10.0,10.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0,0)

group1= starsplatter.StarBunch()
group1.set_nstars(4)
group1.set_coords(0,(0.0,0.0,0.0))
group1.set_coords(1,(5.0,0.0,0.0))
group1.set_coords(2,(0.0,5.0,0.0))
group1.set_coords(3,(0.0,0.0,5.0))
group1.set_bunch_color((0.0,1.0,1.0,1.0))

group2= starsplatter.StarBunch()
group2.set_nstars(3)
group2.set_coords(0,(-5.0,0.0,0.0))
group2.set_coords(1,(0.0,-5.0,0.0))
group2.set_coords(2,(0.0,0.0,-5.0))
group2.set_bunch_color((1.0,0.0,1.0,1.0))

circlegroup= starsplatter.StarBunch()
circlegroup.set_nstars(3)
circlegroup.set_coords(0,(-5.0,0.0,0.0))
circlegroup.set_coords(1,(0.0,-5.0,0.0))
circlegroup.set_coords(2,(0.0,0.0,-5.0))
circlegroup.set_bunch_color((1.0,0.0,0.0,1.0))

myren1= starsplatter.StarSplatter()
myren2= starsplatter.StarSplatter()
myren3= starsplatter.StarSplatter()

for r,g in [(myren1,group1),
            (myren2,group2),
            (myren3,circlegroup)]:
    r.set_image_dims(640,480)
    r.set_camera(mycam)
    r.set_exposure_type( starsplatter.StarSplatter.ET_LINEAR )
    #r.set_debug(1)

    g.set_density(0.3)
    r.add_stars(g)

group1.set_scale_length(0.408)
group2.set_scale_length(1.0)
circlegroup.set_scale_length(1.0)

myren1.set_splat_type(starsplatter.StarSplatter.SPLAT_GAUSSIAN)
myren2.set_splat_type(starsplatter.StarSplatter.SPLAT_SPLINE)
myren3.set_splat_type(starsplatter.StarSplatter.SPLAT_GLYPH_CIRCLE)
img1= myren1.render()
img2= myren2.render()
img3= myren3.render()
img2.add_under(img1)
img3.add_under(img2)
black= starsplatter.rgbImage(img1.xsize(),img1.ysize())
black.clear((0.0,0.0,0.0,1.0))
img3.add_under(black)
img3.save("test.png","png")

