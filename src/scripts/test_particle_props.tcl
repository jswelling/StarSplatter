set gas [ ssplat create_starbunch ]
$gas bunch_color { 0.3 0.4 0.5 }
$gas nstars 16
$gas dump

$gas particle_density 2 0.15
$gas particle_exponent_constant 3 1.2
$gas dump

set baseNProps [ $gas get nprops ]
$gas nprops [expr $baseNProps + 3]

$gas attr color_alg colormap_2d

$gas prop 7 [expr $baseNProps + 1] 123.4
$gas prop 3 [expr $baseNProps + 2] 45.6
$gas prop 5 [expr $baseNProps + 0] 7.8
$gas propname [expr $baseNProps + 0] somePropName
puts "Just set property 0 name to <[$gas get propname [expr $baseNProps + 0]]>"
puts "Property 1 should have no name: <[$gas get propname [expr $baseNProps + 1]]>"
puts "Property 3 does not even exist: <[$gas get propname [expr $baseNProps + 3]]>"
$gas dump

puts "Got attr value [ $gas get attr color_alg ]"

set i 0
while { $i < 4 } {
    set j 0
    while { $j < 4 } {
	set which_star [expr ( 4 * $i ) + $j]
	$gas coords $which_star [ list [expr ( 0.5 * $i ) - 0.75] [expr ( 0.5 * $j ) - 0.75] 0.0 ]
	$gas particle_density $which_star [expr ( 0.001 * $i ) + 0.0005]
	$gas particle_exponent_constant $which_star [expr ( 1000.0 * $j ) + 500.0 ]
	incr j 1
    }
    incr i 1
}
$gas fulldump
