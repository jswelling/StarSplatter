set mycam [ ssplat create_camera ]
#$mycam from { 0.0 0.0 4.0 }
$mycam from { 1.0 0.0 4.0 }
$mycam hither 0.1
$mycam yon 7.0
#$mycam projection parallel
set x_axis [ ssplat create_starbunch ]
set y_axis [ ssplat create_starbunch ]
set z_axis [ ssplat create_starbunch ]
set myren [ ssplat create_renderer ]
$myren debug on
#$myren image_size 80 60
$myren image_size 640 480
$myren camera $mycam
$x_axis bunch_color { 1.0 0.0 0.0 }
$x_axis nstars 3
$x_axis density 0.001
$x_axis exponent_constant 3000.0
$x_axis coords 0 { 0.0 0.0 0.0 }
$x_axis coords 1 { 0.5 0.0 0.0 }
$x_axis coords 2 { 1.0 0.0 0.0 }
$y_axis bunch_color { 0.0 1.0 0.0 }
$y_axis nstars 3
$y_axis density 0.001
$y_axis exponent_constant 3000.0
$y_axis coords 0 { 0.0 0.0 0.0 }
$y_axis coords 1 { 0.0 0.5 0.0 }
$y_axis coords 2 { 0.0 1.0 0.0 }
$z_axis bunch_color { 0.0 0.0 1.0 }
$z_axis nstars 3
$z_axis density 0.001
$z_axis exponent_constant 3000.0
$z_axis coords 0 { 0.0 0.0 0.0 }
$z_axis coords 1 { 0.0 0.0 0.5 }
$z_axis coords 2 { 0.0 0.0 1.0 }
$myren render test.tiff "$x_axis $y_axis $z_axis"
#set i 0
#while { $i < 120 } {
#    set fname [ format "/usr2/welling/frames/frame_%04d.tiff" $i ]
#    $myren render $fname "$gas"
#    $myren rotate 3.0 { 0.0 1.0 0.0 }
#    incr i 1
#}

