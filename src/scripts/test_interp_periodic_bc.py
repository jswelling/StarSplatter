#! /usr/bin/env python
import sys
import os
import math
import starsplatter

worldBB= starsplatter.gBoundBox(1.0,2.0,3.0,
                                3.0,7.0,10.0)

def createBBoxCamera(bbox,fov=35.0):
    atPt= bbox.center()
    d= bbox.xmax()-bbox.xmin()
    diagSqr= d*d
    d= bbox.ymax()-bbox.ymin()
    diagSqr += d*d
    d= bbox.zmax()-bbox.zmin()
    diagSqr += d*d
    diag= math.sqrt(diagSqr)
    fromPt= (atPt[0],atPt[1],atPt[2]+3*diag)
    mycam= starsplatter.Camera(fromPt,atPt,(0.0,1.0,0.0),
                               fov, -0.1*diag, -5*diag)
    return mycam

def drawLine( sb, startIndex, n, startPt, endPt ):
     sx= endPt[0]-startPt[0]
     sy= endPt[1]-startPt[1]
     sz= endPt[2]-startPt[2]
     dx= sx/(n-1)
     dy= sy/(n-1)
     dz= sz/(n-1)
     for i in xrange(n):
          sb.set_coords(i+startIndex,(startPt[0]+i*dx,
                                      startPt[1]+i*dy,
                                      startPt[2]+i*dz))

def createBBoxFrame(sb, bbox):
     drawLine(axes, 0, 100,
              (bbox.xmin(),bbox.ymin(),bbox.zmin()),
              (bbox.xmax(),bbox.ymin(),bbox.zmin())
              )
     drawLine(axes, 100, 100,
              (bbox.xmin(),bbox.ymin(),bbox.zmin()),
              (bbox.xmin(),bbox.ymax(),bbox.zmin())
              )
     drawLine(axes, 200, 100,
              (bbox.xmin(),bbox.ymin(),bbox.zmin()),
              (bbox.xmin(),bbox.ymin(),bbox.zmax())
              )

     drawLine(axes, 300, 100,
              (bbox.xmax(),bbox.ymin(),bbox.zmin()),
              (bbox.xmax(),bbox.ymax(),bbox.zmin()),
              )
     drawLine(axes, 400, 100,
              (bbox.xmax(),bbox.ymin(),bbox.zmin()),
              (bbox.xmax(),bbox.ymin(),bbox.zmax())
              )

     drawLine(axes, 500, 100,
              (bbox.xmin(),bbox.ymax(),bbox.zmin()),
              (bbox.xmin(),bbox.ymax(),bbox.zmax())
              )
     drawLine(axes, 600, 100,
              (bbox.xmin(),bbox.ymax(),bbox.zmin()),
              (bbox.xmax(),bbox.ymax(),bbox.zmin())
              )

     drawLine(axes, 700, 100,
              (bbox.xmin(),bbox.ymin(),bbox.zmax()),
              (bbox.xmax(),bbox.ymin(),bbox.zmax())
              )
     drawLine(axes, 800, 100,
              (bbox.xmin(),bbox.ymin(),bbox.zmax()),
              (bbox.xmin(),bbox.ymax(),bbox.zmax())
              )

     drawLine(axes, 900, 100,
              (bbox.xmax(),bbox.ymax(),bbox.zmin()),
              (bbox.xmax(),bbox.ymax(),bbox.zmax())
              )
     drawLine(axes, 1000, 100,
              (bbox.xmin(),bbox.ymax(),bbox.zmax()),
              (bbox.xmax(),bbox.ymax(),bbox.zmax())
              )
     drawLine(axes, 1100, 100,
              (bbox.xmax(),bbox.ymin(),bbox.zmax()),
              (bbox.xmax(),bbox.ymax(),bbox.zmax())
              )

bunchList= []

ctr= worldBB.center()

mycam= createBBoxCamera(worldBB,15.0)

axes= starsplatter.StarBunch(1200)
axes.set_time(0.0);
axes.set_bunch_color((1.0,0.0,0.0,1.0))
createBBoxFrame(axes,worldBB)

bunchList.append(axes)

group1= starsplatter.StarBunch(1)
group1.set_time(0.0)
group1.set_coords(0,(worldBB.xmin()+0.4,worldBB.ymin()+0.7,ctr[2]))
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

bunchList.append(group1)

group2= starsplatter.StarBunch(1)
group2.set_time(1.0);
group2.set_coords(0,(worldBB.xmax()-0.3,worldBB.ymin()+0.9,ctr[2]))
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

bunchList.append(group2)

#group1.set_attr(starsplatter.StarBunch.DEBUG_LEVEL,1)
interpolated= None
nsteps= 30
for i in xrange(nsteps):
     intBunch= starsplatter.starbunch_interpolate_periodic_bc(group1, group2,
                                                              1.0*(i+1)/(nsteps+1),
                                                              worldBB)
     if (interpolated==None): interpolated= intBunch
     else: interpolated.copy_stars(intBunch)


interpolated.set_colormap1D( [(1.0,0.0,0.0,1.0),(0.0,1.0,0.0,1.0)],
                             1.7, 2.2 )
interpolated.set_attr(starsplatter.StarBunch.COLOR_ALG,
                      starsplatter.StarBunch.CM_COLORMAP_1D)
massIndex= interpolated.get_prop_index_by_name("mass")
interpolated.set_attr(starsplatter.StarBunch.COLOR_PROP1, massIndex)
print "interpolated follows"
interpolated.dump(sys.stdout,1)

bunchList.append(interpolated)

myren= starsplatter.StarSplatter()
#myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_AUTO )
#myren.set_exposure_type( starsplatter.StarSplatter.ET_LINEAR )
myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )

myren.set_image_dims(512,512)
myren.set_camera(mycam)
#myren.set_debug(1)

for sb in bunchList:
    sb.set_density(0.1)
    sb.set_scale_length(0.02)
    myren.add_stars(sb)

img= myren.render()
img.save("test.png","png")


