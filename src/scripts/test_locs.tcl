set mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 3.0 }
$mycam hither 1.0
$mycam yon 5.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set myren [ ssplat create_renderer ]
$myren image_size 640 480
$myren camera $mycam
$gas bunch_color { 1.0 1.0 1.0 }
$gas nstars 7
$gas density 0.0003
$gas exponent_constant 3000.0
$gas coords 0 { 0.0 0.0 0.0 }
$gas coords 1 { 0.5 0.0 0.0 }
$gas coords 2 { -0.5 0.0 0.0 }
$gas coords 3 { 0.0 0.5 0.0 }
$gas coords 4 { 0.0 -0.5 0.0 }
$gas coords 5 { 0.0 0.0 0.5 }
$gas coords 6 { 0.0 0.0 -0.5 }
$myren render test.tiff "$gas"
#set i 0
#while { $i < 120 } {
#    set fname [ format "/usr2/welling/frames/frame_%04d.tiff" $i ]
#    $myren render $fname "$gas"
#    $myren rotate 3.0 { 0.0 1.0 0.0 }
#    incr i 1
#}

