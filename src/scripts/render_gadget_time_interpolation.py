#! /usr/bin/env python
import sys, os, math
import starsplatter

#######################
# This script is similar to render_gadget_cmap.py but
# produces images interpolated in time between the two inputs.
# Only the gas component is rendered for simplicity.
#######################

# Note how this function uses the bunch's 'crop' method to clip
# away particles that lie outside the bounding box
def cropBunchTo(sb,sbName,cropBBox):
    oldN= sb.nstars()
    sb.crop((cropBBox.xmin(),cropBBox.ymin(),cropBBox.zmin()),
            (1.0,0.0,0.0))
    sb.crop((cropBBox.xmax(),cropBBox.ymax(),cropBBox.zmax()),
            (-1.0,0.0,0.0))
    sb.crop((cropBBox.xmin(),cropBBox.ymin(),cropBBox.zmin()),
            (0.0,1.0,0.0))
    sb.crop((cropBBox.xmax(),cropBBox.ymax(),cropBBox.zmax()),
            (0.0,-1.0,0.0))
    sb.crop((cropBBox.xmin(),cropBBox.ymin(),cropBBox.zmin()),
            (0.0,0.0,1.0))
    sb.crop((cropBBox.xmax(),cropBBox.ymax(),cropBBox.zmax()),
            (0.0,0.0,-1.0))
    if sbName!=None:
        print("%s cropped from %d to %d stars"%(sbName,oldN,sb.nstars()))

# This utility makes a complete copy of a StarBunch.
def bunch_clone(sb):
    result= starsplatter.StarBunch()
    result.set_nprops(sb.nprops())
    for i in range(sb.nprops()):
        result.allocate_next_free_prop_index(sb.propName(i))
    result.copy_stars(sb)
    return result

def setBunchProps( gas, meanMassByType ):
    #Set the rendering properties of the particles
    for (name,bunch) in [("gas",gas)]:

        print("Setting properties for %s"%name)
        
        # First we set the smoothing length based
        bunch.set_scale_length(1.0) # global scale
        smoothingLengthId= bunch.get_prop_index_by_name("SmoothingLength")
        if smoothingLengthId>=0: # which means it is present
            for i in range(bunch.nstars()):
                bunch.set_scale_length(i,bunch.prop(i,smoothingLengthId))
                
        # Now we set the optical density
        bunch.set_density(1.0/meanMassByType[name]) # global scale
        massId= bunch.get_prop_index_by_name("Mass")
        if massId>=0: # which means it is present
            for i in range(bunch.nstars()):
                bunch.set_density(i,bunch.prop(i,massId))

    # Set the coloring of the gas with a colormap
    gas.set_bunch_color((1.0,1.0,1.0,1.0))
    internalEnergyID= gas.get_prop_index_by_name("InternalEnergy")
    if internalEnergyID<0:
        sys.exit("Input dataset gas bunch has no internal energy property!")
    gas.set_attr(starsplatter.StarBunch.COLOR_ALG,
                 starsplatter.StarBunch.CM_COLORMAP_1D)
    gas.set_attr(starsplatter.StarBunch.COLOR_PROP1,internalEnergyID);
    # The colormap divides the range up into equal parts, ending each
    # part with the given RGBA value.  So for this colormap there are
    # three linear segments: black, red, orange, white.
    orangeHotColors= [(0.0,0.0,0.0,0.0),
                      (0.5,0.0,0.0,0.5),
                      (1.0,0.5,0.0,1.0),
                      (1.0,1.0,1.0,1.0)]
    # The last two entries are the endpoints of the colormap, which you
    # can find by printing out the bunch with its 'dump' method.
    gas.set_colormap1D(orangeHotColors,
                       0.0, 500000.0)

    print("Done setting properties")


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

if (len(sys.argv)<3):
    sys.exit("You must supply two filenames as arguments.");

fname1= sys.argv[1]
fname2= sys.argv[2]

# We'll clip the data to this bounding box, in order to show handling 
# of cilpped-out points.  You'll probably want to adjust this based
# on the bounds of the data you are rendering.
sliceBBox= starsplatter.gBoundBox(0.0,0.0,10000.0,
                                  33750.0,33750.0,11000.0)

# Load the data.
# We can avoid loading any particular particle type by passing None for
# that bunch.
halo= None
disk= None
bulge= None
stars= None
bndry= None
gas1= starsplatter.StarBunch()
gas2= starsplatter.StarBunch()

# These are the mean masses for the various particle types, 
# useful in getting a good balance of optical densities
meanMassByType= { "gas":0.000371245 }
for gas,fname,bunchname in [(gas1,fname1,"gas1"),(gas2,fname2,"gas2")]:
    infile= open(fname,"r")
    outList= starsplatter.load_gadget(infile,[gas,halo,disk,bulge,stars,bndry])
    infile.close()

# Create versions restricted to particles within the bounding box
gas1_cropped= bunch_clone(gas1)
cropBunchTo(gas1_cropped,"gas1_cropped",sliceBBox)
gas2_cropped= bunch_clone(gas2)
cropBunchTo(gas2_cropped,"gas2_cropped",sliceBBox)

# Some particles will have moved into or out of the box during the
# time interval we're dealing with.  We must find their records
# within the whole corresponding dataset and add them to the cropped
# bunches.  This is done based in the ID numbers in the dataset,
# which were provided by the input data file.
starsplatter.identify_unshared_ids(gas1_cropped,gas2_cropped)
print("gas1_cropped now has %d invalid"%gas1_cropped.ninvalid())
print("gas2_cropped now has %d invalid"%gas2_cropped.ninvalid())

# Now the missing IDs have been added, but they are invalid- there
# is no data associated with them.  Fill in that data if it is
# available.
print("Searching for missing particles")
gas1.set_valid(0,1) # this creates a 'validity' property
if gas1_cropped.fill_invalid_from(gas1)==0:
    print("fill_invalid_from(gas1_cropped) failed!")
    sys.exit(-1)
print("gas1_cropped now has %d invalid"%gas1_cropped.ninvalid())
gas2.set_valid(0,1) # this creates a 'validity' property
if gas2_cropped.fill_invalid_from(gas2)==0:
    print("fill_invalid_from(gas2_cropped) failed!")
    sys.exit(-1)
print("gas2_cropped now has %d invalid"%gas2_cropped.ninvalid())

# Set the rendering properties of the bunches
setBunchProps( gas1_cropped, meanMassByType )
setBunchProps( gas2_cropped, meanMassByType )

#
# Take a break to set up the camera and renderer
#

mycam= createBBoxCamera(sliceBBox,20.0)
myren= starsplatter.StarSplatter()
myren.set_image_dims(512, 512)
myren.set_camera(mycam)
myren.set_exposure_scale(0.5)
myren.set_exposure_type( starsplatter.StarSplatter.ET_LOG_HSV_AUTO )
myren.set_splat_type( starsplatter.StarSplatter.SPLAT_SPLINE ) 

#
# Create new starbunches by interpolating at particular times...
#

# velscale is a factor to adjust units between positions and velocities.
# For example, if positions are in kiloparsecs, velocities are in
# kilometers per second and times are in seconds, velscale would be
# 1.0/(30.86e15).
velscale= 1.0/(30.86e15)

# alpha below is interpolated between these times
gas1_cropped.set_time(0.0)
gas2_cropped.set_time(1.0)
for i in range(10):
    alpha= float(i)/float(10)
    interpolatedGas= starsplatter.starbunch_interpolate(gas1_cropped,
                                                        gas2_cropped,
                                                        alpha,
                                                        velscale)
    # But interpolated particles may not lie in the bounding box!
    # We need to crop them.
    cropBunchTo(interpolatedGas,"interpolatedGas",sliceBBox)
    
    # Render the particles with the given camera, and save the
    # image
    myren.add_stars(interpolatedGas)
    img= myren.render()
    black= starsplatter.rgbImage(img.xsize(),img.ysize())
    black.clear((0.0,0.0,0.0,1.0))
    img.add_under(black)
    fname= "frame_%03d.png"%i
    img.save(fname,"png")
    print("wrote %s"%fname)

    # Clear the stars out of the renderer; we don't want them
    # to accumulate!
    myren.clear_stars()
