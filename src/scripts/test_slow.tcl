set mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 3.0 }
$mycam hither 1.0
$mycam yon 5.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set stars [ ssplat create_starbunch ]
set dark [ ssplat create_starbunch ]
ssplat load_tipsy_box test.tipsy $gas $stars $dark
set myren [ ssplat create_renderer ]
$myren image_size 640 480
#$myren image_size 64 48
$myren camera $mycam
$myren debug on
# The following call is ignored as of version 2.1.0
$myren force_slow_splat_method true
$gas bunch_color { 1.0 0.0 0.0 }
$gas density 0.0003
$gas exponent_constant 3000.0
$stars bunch_color { 0.0 1.0 0.0 }
$stars density 0.0003
$stars exponent_constant 3000.0
$dark bunch_color { 0.0 0.0 1.0 }
$dark density 0.0003
$dark exponent_constant 3000.0
$myren render test.tiff "$gas $stars $dark"
