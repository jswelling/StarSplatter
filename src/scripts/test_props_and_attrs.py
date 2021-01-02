#! /usr/bin/env python
import sys
import os
import starsplatter

gas= starsplatter.StarBunch()
gas.set_bunch_color((0.3,0.4,0.5,0.1))
gas.set_nstars(16)
gas.dump(sys.stderr)

gas.set_density(2,0.15)
gas.set_exp_constant(3,1.2)
gas.dump(sys.stderr)

baseNProps= gas.nprops()
gas.set_nprops(baseNProps+3)

gas.set_attr(starsplatter.StarBunch.COLOR_ALG,
             starsplatter.StarBunch.CM_COLORMAP_2D)
gas.set_prop(7,baseNProps+1,123.4)
gas.set_prop(3,baseNProps+2,45.6)
gas.set_prop(5,baseNProps+0,7.8)
gas.set_propName(0+baseNProps,"somePropName")
print "Just set property %d+0 name to <%s>"%\
      (baseNProps,gas.propName(baseNProps+0))
print "Property %d+1 has no name: <%s>"%\
      (baseNProps,gas.propName(baseNProps+1))
print "Property %d+3 does not even exist: <%s>"%\
      (baseNProps,gas.propName(baseNProps+3))
gas.dump(sys.stderr)

print "Got attr value %d"%gas.attr(starsplatter.StarBunch.COLOR_ALG)

for i in xrange(0,4):
    for j in xrange(0,4):
        which_star= (4*i)+j
        gas.set_coords(which_star,(0.5*i - 0.75, 0.5*j - 0.75, 0.0))
        gas.set_density(which_star, 0.001*i + 0.0005)
        gas.set_exp_constant(which_star,1000.0*j + 5000.0)

gas.dump(sys.stderr,1)

