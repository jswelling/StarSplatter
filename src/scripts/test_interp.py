#! /usr/bin/env python
import sys
import os
import starsplatter

def writeout(lblstr, sb):
    print("%s follows"%lblstr)
    sb.dump(sys.stdout,1)
    for i in range(sb.nstars()):
        print("%d: id= %d, valid= %d"%(i,sb.id(i),sb.valid(i)))
    print("%s has ninvalid %d"%(lblstr,group1.ninvalid()))
    

############
# Main
############
mycam= starsplatter.Camera((0.0,0.0,30.0),
                           (0.0,0.0,0.0),
                           (0.0,1.0,0.0),
                           35.0,-20.0,-40.0)

group1= starsplatter.StarBunch()
group1.set_time(0.3)
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
group1.set_valid(0,1)
group1.set_valid(1,1)
group1.set_valid(2,0)
group1.set_valid(3,0)
writeout("Raw group1",group1)
group1.sort_ascending_by_id()
writeout("Sorted group1",group1)
    
group2= starsplatter.StarBunch()
group2.set_time(1.2)
group2.set_nstars(3)
group2.set_coords(0,(-5.0,0.0,0.0))
group2.set_coords(1,(0.0,-5.0,0.0))
group2.set_coords(2,(0.0,0.0,-5.0))
group2.set_bunch_color((1.0,0.0,0.0,1.0))
group2.set_id(0,21)
group2.set_id(1,5)
group2.set_id(2,17)
#group2.dump(sys.stderr)

group3= starsplatter.StarBunch()
group3.set_nstars(3)
group3.set_coords(0,(1.0,0.0,0.0))
group3.set_coords(1,(0.0,1.0,0.0))
group3.set_coords(2,(0.0,0.0,1.0))
group3.set_bunch_color((0.0,1.0,0.0,1.0))
group3.set_id(0,4)
group3.set_id(1,18)
group3.set_id(2,5)
group3.set_valid(0,1)
group3.set_valid(1,1)
group3.set_valid(2,1)
writeout("Group 3",group3)

starsplatter.identify_unshared_ids(group1,group2)
writeout("Group 1",group1)
writeout("Group 2",group2)
group1.fill_invalid_from(group3)
writeout("Filled group 1",group1)
group2.fill_invalid_from(group3)
writeout("Filled group 2",group1)

gp1_vxProp= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_X_NAME)
gp1_vyProp= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Y_NAME)
gp1_vzProp= group1.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Z_NAME)
gp1_fooProp= group1.allocate_next_free_prop_index("foo")
gp1_barProp= group1.allocate_next_free_prop_index("bar")
for i in range(group1.nstars()):
    group1.set_prop(i,gp1_vxProp,12.0+0.1*i)
    group1.set_prop(i,gp1_vyProp,-4.0-0.1*i)
    group1.set_prop(i,gp1_vzProp,3.0+0.1*i)
    group1.set_prop(i,gp1_fooProp,1.23)
    group1.set_prop(i,gp1_barProp,4.56)


gp2_vxProp= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_X_NAME)
gp2_vyProp= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Y_NAME)
gp2_vzProp= group2.allocate_next_free_prop_index(starsplatter.StarBunch.VEL_Z_NAME)
gp2_fooProp= group2.allocate_next_free_prop_index("foo")
for i in range(group2.nstars()):
    group2.set_prop(i,gp2_vxProp,2.0+0.1*i)
    group2.set_prop(i,gp2_vyProp,-14.0-0.1*i)
    group2.set_prop(i,gp2_vzProp,30.0+0.1*i)
    group2.set_prop(i,gp2_fooProp,7.89)

group1.set_attr(starsplatter.StarBunch.DEBUG_LEVEL,1)
interpBunch= starsplatter.starbunch_interpolate(group1, group2, 0.73)
print("result was %s"%interpBunch)

