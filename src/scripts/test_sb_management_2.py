#! /usr/bin/env python
import sys
import os
import starsplatter

def mkBunch( dx, t ):
    gp= starsplatter.StarBunch()
    vxId= gp.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_X_NAME)
    vyId= gp.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Y_NAME)
    vzId= gp.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Z_NAME)
    gp.set_time(t)
    gp.set_nstars(4)
    gp.set_coords(0,(0.0+dx,0.0,0.0))
    gp.set_prop(0,vxId,0.0)
    gp.set_prop(0,vyId,0.0)
    gp.set_prop(0,vzId,0.0)
    gp.set_id(0,0)
    gp.set_coords(1,(5.0+dx,0.0,0.0))
    gp.set_prop(1,vxId,0.0)
    gp.set_prop(1,vyId,0.0)
    gp.set_prop(1,vzId,0.0)
    gp.set_id(1,0)
    gp.set_coords(2,(0.0+dx,5.0,0.0))
    gp.set_prop(2,vxId,0.0)
    gp.set_prop(2,vyId,0.0)
    gp.set_prop(2,vzId,0.0)
    gp.set_id(2,0)
    gp.set_coords(3,(0.0+dx,0.0,5.0))
    gp.set_prop(3,vxId,0.0)
    gp.set_prop(3,vyId,0.0)
    gp.set_prop(3,vzId,0.0)
    gp.set_id(3,0)
    gp.set_bunch_color((0.1*dx,1.0-0.1*dx,1.0,1.0))
    return gp

mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)


myren= starsplatter.StarSplatter()
myren2= starsplatter.StarSplatter()

myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )
myren.set_image_dims(640,480)
myren.set_camera(mycam)

myren2.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )
myren2.set_image_dims(480,640)
myren2.set_camera(mycam)

sb1= mkBunch(0.1,0.0)
sb2= mkBunch(0.3,1.0)

#sb1.set_attr(starsplatter.StarBunch.DEBUG_LEVEL,1)

for i in xrange(0,5):
    interp= starsplatter.starbunch_interpolate(sb1,sb2,0.2*i,1.0)
    sys.stderr.write("done interpolating\n")
    sys.stderr.write("%d ...\n"%i)
    myren.add_stars(interp)
    sys.stderr.write("done adding to 1\n")
    if i%2 == 0:
        myren2.add_stars(interp)
        sys.stderr.write("done adding to 2\n")
    else:
        sys.stderr.write("not adding to 2\n")
    img= myren.render()
    sys.stderr.write("clearing 1\n")
    myren.clear_stars()
    sys.stderr.write("done clearing 1\n")
    img.save("test.png","png")
    img= myren2.render()
    img.save("test2.png","png")
    sys.stderr.write("clearing 2\n")
    myren2.clear_stars()
    sys.stderr.write("done clearing 2\n")
    del interp
    sys.stderr.write("done freeing interp\n")


sys.stderr.write("done!\n")





