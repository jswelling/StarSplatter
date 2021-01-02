set mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 3.0 }
$mycam hither 1.0
$mycam yon 5.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set stars [ ssplat create_starbunch ]
set dark [ ssplat create_starbunch ]
ssplat load_tipsy_box ../data/test.tipsy $gas $stars $dark
set myren [ ssplat create_renderer ]
$myren image_size 320 240
$myren camera $mycam
$gas bunch_color { 1.0 0.0 0.0 }
$gas density 0.0003
$gas exponent_constant 3000.0
$stars bunch_color { 0.0 1.0 0.0 }
$stars density 0.0003
$stars exponent_constant 3000.0
$dark bunch_color { 0.0 0.0 1.0 }
$dark density 0.0003
$dark exponent_constant 3000.0
set i 0
while { $i < 120 } {
    set fname [ format "/usr2/welling/frames/frame_%04d.tiff" $i ]
    $myren render $fname "$gas $stars $dark"
    $myren rotate 3.0 { 0.0 1.0 0.0 }
    puts "$i finished"
    incr i 1
}
