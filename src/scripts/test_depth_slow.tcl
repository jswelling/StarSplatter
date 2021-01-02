set mycam [ ssplat create_camera ]
#$mycam from { 0.0 0.0 4.0 }
$mycam from { 0.1 0.1 4.0 }
$mycam hither 0.1
$mycam yon 7.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set myren [ ssplat create_renderer ]
$myren debug on
$myren image_size 80 60
#$myren image_size 160 120
$myren camera $mycam
# the following call is ignored as of version 2.1.0
$myren force_slow_splat_method true
$gas bunch_color { 1.0 1.0 1.0 }
$gas nstars 15
$gas density 0.001
$gas exponent_constant 3000.0
$gas coords 0 { -1.0 -0.1 -2.0 }
$gas coords 1 { -0.8 -0.1 -1.6 }
$gas coords 2 { -0.6 -0.1 -1.2 }
$gas coords 3 { -0.4 -0.1 -0.8}
$gas coords 4 { -0.2 -0.1 -0.4 }
$gas coords 5 { 0.0 -0.1 0.0 }
$gas coords 6 { 0.2 -0.1 0.4 }
$gas coords 7 { 0.4 -0.1 0.8 }
$gas coords 8 { 0.6 -0.1 1.2 }
$gas coords 9 { 0.8 -0.1 1.6 }
$gas coords 10 { 1.0 -0.1 2.0 }
$gas coords 11 { 0.6 0.1 2.4 }
$gas coords 12 { 0.2 0.1 2.8 }
$gas coords 13 { -0.2 0.1 3.2 }
$gas coords 14 { 0.0 0.2 3.6 }
$myren render test.tiff "$gas"
#set i 0
#while { $i < 120 } {
#    set fname [ format "/usr2/welling/frames/frame_%04d.tiff" $i ]
#    $myren render $fname "$gas"
#    $myren rotate 3.0 { 0.0 1.0 0.0 }
#    incr i 1
#}

