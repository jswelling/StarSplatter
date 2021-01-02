set big_galaxy 0
while { $big_galaxy < 20 } {
    set bdisk($big_galaxy) [ ssplat create_starbunch ]
    $bdisk($big_galaxy) nstars 60000
    lappend all_bunches $bdisk($big_galaxy)
    incr big_galaxy 1
}

set small_galaxy 0
while { $small_galaxy < 80 } {
    set sdisk($small_galaxy) [ ssplat create_starbunch ]
    $sdisk($small_galaxy) nstars 6000
    lappend all_bunches $sdisk($small_galaxy)
    incr small_galaxy 1
}
set infile [ open dubinski/cluster/rv009.3700.splats ]
puts "beginning loads"
ssplat load $infile binary xyzdkxyzdk $all_bunches
puts "loads complete"
close $infile

set mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 1000.0 }
$mycam hither 100.0
$mycam yon 5000.0

set myren [ ssplat create_renderer ]
$myren image_size 640 480
$myren camera $mycam
$myren debug on
