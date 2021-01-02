set mycam [ ssplat create_camera ]
$mycam from {-5.0 -9.0 3.0}
$mycam up {0.0 0.0 1.0}
$mycam hither 1.0
$mycam yon 50.0
$mycam fovea 35.0
#$mycam projection parallel
# I think disk1 and bulge1 are Milky Way, disk2 and bulge2 are Andromeda
set disk1 [ ssplat create_starbunch ]
set disk2 [ ssplat create_starbunch ]
set bulge1 [ ssplat create_starbunch ]
set bulge2 [ ssplat create_starbunch ]
if {[llength $argv] < 1} {
    error "You must supply a filename as an argument."
}
puts [ ssplat load_dubinski $argv "$disk1 $disk2 $bulge1 $bulge2" ] 
set myren [ ssplat create_renderer ]
$myren image_size 640 480
$myren camera $mycam
$myren exposure_rescale_type log_auto
$myren exposure 0.5
$myren debug on
$disk1 bunch_color { 0.8 0.5 1.0 1.0}
$disk1 density 0.00002
$disk1 exponent_constant 1000.0
$disk1 dump
$disk2 bunch_color { 0.5 0.8 0.0 1.0}
$disk2 density 0.00002
$disk2 exponent_constant 1000.0
$bulge1 bunch_color { 0.8 0.7 0.5 1.0}
$bulge1 density 0.000006
$bulge1 exponent_constant 1000.0
$bulge2 bunch_color { 0.8 0.7 0.5 1.0}
$bulge2 density 0.000006
$bulge2 exponent_constant 1000.0
$myren render test.png "$disk1 $disk2 $bulge1 $bulge2"




