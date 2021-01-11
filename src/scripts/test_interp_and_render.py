#! /usr/bin/env python
import sys
import os
import starsplatter

mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)

group1= starsplatter.StarBunch()
group1.set_time(0.0);
group1.set_nstars(1)
group1.set_coords(0,(-5.0,0.0,0.0))
group1.set_id(0,0);
g1vx= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_X_NAME)
g1vy= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Y_NAME)
g1vz= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Z_NAME)
gp1Mass= group1.allocate_next_free_prop_index("mass")
group1.set_prop(0,g1vx,0.0);
group1.set_prop(0,g1vy,10.0);
group1.set_prop(0,g1vx,0.0);
group1.set_prop(0,gp1Mass,1.7)
group1.set_bunch_color((1.0,1.0,1.0,1.0))

group2= starsplatter.StarBunch()
group2.set_time(1.0);
group2.set_nstars(1)
group2.set_coords(0,(5.0,0.0,0.0))
group2.set_id(0,0);
g2vx= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_X_NAME)
g2vy= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Y_NAME)
g2vz= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Z_NAME)
gp2Mass= group2.allocate_next_free_prop_index("mass")
group2.set_prop(0,g2vx,0.0);
group2.set_prop(0,g2vy,10.0);
group2.set_prop(0,g2vx,0.0);
group2.set_prop(0,gp2Mass,2.2)
group2.set_bunch_color((1.0,1.0,1.0,1.0))

interpolated= None
for i in range(9):
     intBunch= starsplatter.starbunch_interpolate(group1, group2, 0.1*(i+1))
     if (interpolated==None): interpolated= intBunch
     else: interpolated.copy_stars(intBunch)

group1.set_bunch_color((1.0,0.0,0.0,1.0))
group2.set_bunch_color((0.0,1.0,0.0,1.0))

interpolated.set_colormap1D( [(1.0,0.0,0.0,1.0),(0.0,1.0,0.0,1.0)],
                             1.7, 2.2 )
interpolated.set_attr(starsplatter.StarBunch.COLOR_ALG,
                      starsplatter.StarBunch.CM_COLORMAP_1D)
massIndex= interpolated.get_prop_index_by_name("mass")
interpolated.set_attr(starsplatter.StarBunch.COLOR_PROP1, massIndex)
print("interpolated follows")
interpolated.dump(sys.stderr,1)
                             
bunchList= [group1, group2, interpolated]

myren= starsplatter.StarSplatter()
#myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_AUTO )
myren.set_exposure_type( starsplatter.StarSplatter.ET_LINEAR )

myren.set_image_dims(640,480)
myren.set_camera(mycam)
#myren.set_debug(1)

for sb in bunchList:
    sb.set_density(0.03)
    sb.set_scale_length(0.1)
    myren.add_stars(sb)

img= myren.render()
img.save("test.png","png")

