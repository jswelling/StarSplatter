#! /usr/bin/env python
import sys
import os
import starsplatter

mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)

group1= starsplatter.StarBunch()
#group1.set_attr(starsplatter.StarBunch.DEBUG_LEVEL,1)
group1.set_nstars(4)
group1.set_coords(0,(0.0,0.0,0.0))
group1.set_coords(1,(5.0,0.0,0.0))
group1.set_coords(2,(0.0,5.0,0.0))
group1.set_coords(3,(0.0,0.0,5.0))
group1.set_bunch_color((1.0,1.0,1.0,1.0))
group1.set_id(0,17)
group1.set_id(1,18)
group1.set_id(2,21)
group1.set_id(3,4)
#group1.dump(sys.stderr)

group2= starsplatter.StarBunch()
group2.set_nstars(2)
group2.set_coords(0,(-5.0,0.0,0.0))
group2.set_coords(1,(0.0,-5.0,0.0))
group2.set_bunch_color((1.0,0.0,0.0,1.0))
group2.set_id(0,21)
group2.set_id(1,5)
#group2.dump(sys.stderr)

print "Group 1 follows"
group1.dump(sys.stderr)
for i in xrange(group1.nstars()):
    print "%d: id= %d, valid= %d"%(i,group1.id(i),group1.valid(i))
print "Group 1 has ninvalid %d"%group1.ninvalid()
print "Group 2 follows"
group2.dump(sys.stderr)
for i in xrange(group2.nstars()):
    print "%d: id= %d, valid= %d"%(i,group2.id(i),group2.valid(i))
print "Group 2 has ninvalid %d"%group2.ninvalid()

group1.copy_stars(group2)
print "Merged group1 follows"
group1.dump(sys.stderr,1)
print "     ",
for j in xrange(group1.nprops()):
    print "%10s"%group1.propName(j),
print ""
for i in xrange(group1.nstars()):
    print "%d"%i,
    for j in xrange(group1.nprops()):
        if group1.propName(j)=="particleID":
            print "%10d"%group1.id(i),
        elif group1.propName(j)=="particleRecValid":
            print "%10d"%group1.valid(i),
        else:
            print "%10g"%group1.prop(i,j),
    print ""
    

group3= starsplatter.StarBunch()
group3.set_nstars(1)
group3.set_coords(0,(0.0,-5.0,0.0))
group3.set_bunch_color((0.0,1.0,0.0,1.0))

group4= starsplatter.StarBunch()
group4.set_nstars(1)
group4.set_coords(0,(0.0,0.0,-5.0))
group4.set_bunch_color((0.0,0.0,1.0,1.0))

print "End of function"

