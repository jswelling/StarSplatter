set mycam [ ssplat create_camera ]
$mycam from {15.0 15.0 40.0}
$mycam up {0.0 1.0 0.0}
$mycam hither 1.0
$mycam yon 150.0
$mycam fovea 45.0
#$mycam projection parallel
set disk1 [ ssplat create_starbunch ]
set disk2 [ ssplat create_starbunch ]
set bulge1 [ ssplat create_starbunch ]
set bulge2 [ ssplat create_starbunch ]
set halo1 [ ssplat create_starbunch ]
set halo2 [ ssplat create_starbunch ]
puts [ ssplat load_dubinski dubinski/SCF_big_merger/coords_tenth_0399.dubinski "$disk1 $disk2 $bulge1 $bulge2 $halo1 $halo2" ] 
set myren [ ssplat create_renderer ]
$myren image_size 512 512
$myren camera $mycam
#$myren log_exposure on
$myren exposure_rescale_type log
$myren exposure 0.5
$myren debug on
#$disk1 bunch_color { 1.0 0.0 0.0 0.1}
$disk1 bunch_color { 0.8 0.5 1.0 0.1}
#$disk1 density 0.00003
$disk1 density 0.001
$disk1 exponent_constant 100.0
#$disk2 bunch_color { 0.0 1.0 0.0 0.1}
$disk2 bunch_color { 0.5 0.8 0.0 0.1}
#$disk2 density 0.00003
$disk2 density 0.001
$disk2 exponent_constant 100.0
#$bulge1 bunch_color { 0.0 0.0 1.0 0.1}
$bulge1 bunch_color { 0.8 0.7 0.5 0.1}
$bulge1 density 0.0001
$bulge1 exponent_constant 100.0
#$bulge2 bunch_color { 1.0 0.0 1.0 0.1}
$bulge2 bunch_color { 0.8 0.7 0.5 0.1}
$bulge2 density 0.0001
$bulge2 exponent_constant 100.0
#$halo1 bunch_color { 1.0 0.0 0.0 0.1}
$halo1 bunch_color { 1.0 0.0 0.0 0.1}
$halo1 density 0.00003
$halo1 exponent_constant 1000.0
#$halo2 bunch_color { 1.0 0.0 0.0 0.1}
$halo2 bunch_color { 1.0 0.0 0.0 0.1}
$halo2 density 0.00003
$halo2 exponent_constant 1000.0

$myren render test.tiff "$disk1 $disk2 $bulge1 $bulge2 $halo1 $halo2"




