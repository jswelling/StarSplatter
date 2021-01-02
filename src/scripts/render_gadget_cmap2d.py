#! /usr/bin/env python
import sys, os, math
import starsplatter

#######################
# This script is equivalent to render_gadget_cmap.py but
# applies a 2D color map to the 'gas' starbunch.
#######################

def setBunchProps( gas, disk, bulge, stars, meanMassByType ):
    #Set the rendering properties of the particles
    for (name,bunch) in [("gas",gas), ("disk",disk),
                         ("bulge",bulge), ("stars",stars)]:

        print "Setting properties for %s"%name

        # First we set the smoothing length based
        bunch.set_scale_length(1.0) # global scale
        smoothingLengthId= bunch.get_prop_index_by_name("SmoothingLength")
        if smoothingLengthId>=0: # which means it is present
            for i in xrange(bunch.nstars()):
                bunch.set_scale_length(i,bunch.prop(i,smoothingLengthId))

        # Now we set the optical density
        bunch.set_density(1.0/meanMassByType[name]) # global scale
        massId= bunch.get_prop_index_by_name("Mass")
        if massId>=0: # which means it is present
            for i in xrange(bunch.nstars()):
                bunch.set_density(i,bunch.prop(i,massId))

    gas.set_bunch_color((0.8, 0.5, 1.0, 0.1))
    disk.set_bunch_color((0.5, 0.5, 0.9, 0.1))
    bulge.set_bunch_color((0.8, 0.7, 0.5, 0.1))
    stars.set_bunch_color((1.0, 1.0, 1.0, 0.3))

    print "Done setting properties"


def createBBoxCamera(bbox,fov=35.0,sepfac=1.0):
    atPt= bbox.center()
    d= bbox.xmax()-bbox.xmin()
    diagSqr= d*d
    d= bbox.ymax()-bbox.ymin()
    diagSqr += d*d
    d= bbox.zmax()-bbox.zmin()
    diagSqr += d*d
    diag= math.sqrt(diagSqr)
    fromPt= (atPt[0]+0.5*sepfac*diag,
             atPt[1]+0.5*sepfac*diag,
             atPt[2]+3*sepfac*diag)
    thiscam= starsplatter.Camera(fromPt,atPt,(0.0,1.0,0.0),
                                 fov, -1*sepfac*diag, -5*sepfac*diag)
    return thiscam

if (len(sys.argv)<2):
    sys.exit("You must supply a filename as an argument.");

# Load the data.
# We can avoid loading any particular particle type by passing None for
# that bunch.
gas= starsplatter.StarBunch()
halo= None
disk= starsplatter.StarBunch()
bulge= starsplatter.StarBunch()
stars= starsplatter.StarBunch()
bndry= None
infile= open(sys.argv[1],"r")
outList= starsplatter.load_gadget(infile,[gas,halo,disk,bulge,stars,bndry])
infile.close()

# Find the bounding box which holds all the particles, printing their
# properties as we go
bbox= None
for (name,bunch) in [("gas",gas), ("disk",disk),
                     ("bulge",bulge), ("stars",stars)]:
    print "%%%% %s: %d particles"%(name,bunch.nstars())
    if (bunch.nstars()>0):
        bunch.dump(sys.stdout)
        if not bbox: bbox= bunch.boundBox()
        else: bbox.union_with(bunch.boundBox())

# These are the mean masses for the various particle types, 
# useful in getting a good balance of optical densities
meanMassByType= { "stars":1.95e-5, "disk":1.0, "bulge":1.0, "gas":3.03e-5 }

# Set preliminary properties
setBunchProps( gas, disk, bulge, stars, meanMassByType )

# Override the coloring of the gas with a colormap
gas.set_bunch_color((1.0,1.0,1.0,1.0)) # this will scale the colormap
internalEnergyID= gas.get_prop_index_by_name("InternalEnergy")
if internalEnergyID<0:
    sys.exit("Input dataset gas bunch has no internal energy property!")
massID= gas.get_prop_index_by_name("Mass")
if massID<0:
    sys.exit("Input dataset gas bunch has no mass property!")
gas.set_attr(starsplatter.StarBunch.COLOR_ALG,
             starsplatter.StarBunch.CM_COLORMAP_2D)
gas.set_attr(starsplatter.StarBunch.COLOR_PROP1,internalEnergyID);
gas.set_attr(starsplatter.StarBunch.COLOR_PROP2,internalEnergyID);
# The 2D colormap is sort of a colormap of colormaps.  Property 1
# controls the location along the inner dimension; property 2
# controls it along the outer dimension.
redBlueShadedColors= [[(1.0,0.0,0.0,1.0),(0.0,0.0,1.0,1.0)],
                      [(1.0,0.3,0.0,1.0),(0.0,0.3,1.0,1.0)]]
# The last two entries are the endpoints of the colormap, which you
# can find by printing out the bunch with its 'dump' method.
gas.set_colormap2D(redBlueShadedColors,
                   0.0, 500000.0,
                   1.0e-5,5.0e-5)

# Set up camera and renderer
mycam= createBBoxCamera(bbox,20.0)
myren= starsplatter.StarSplatter()
myren.set_image_dims(512, 512)
myren.set_camera(mycam)
myren.set_exposure_scale(0.5)

myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )
# If this were not AUTO, we'd need to set bounds:
# myren.set_log_rescale_bounds(1.0e-5, 1.0)

# This splat type is appropriate for Gadget; it looks rather like
# a Gaussian splat with length scaled by 0.408 but has a sharp
# outer edge
myren.set_splat_type( starsplatter.StarSplatter.SPLAT_SPLINE ) 

# Uncomment this to see a histogram of pixel values
# myren.set_debug(1)

# Render the particles with the given camera.
# The image which is produced includes opacity so that it can be
# composited with other images.  We want a simple black background,
# so create one and composite this image on top.
for sb in [gas, disk, bulge, stars]:
    myren.add_stars(sb)
img= myren.render()
black= starsplatter.rgbImage(img.xsize(),img.ysize())
black.clear((0.0,0.0,0.0,1.0))
img.add_under(black)

# Save the image with the given type and name
img.save("test.png","png")
print "wrote test.png"
