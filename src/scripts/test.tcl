set mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 3.0 }
$mycam hither 1.0
$mycam yon 5.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set stars [ ssplat create_starbunch ]
set dark [ ssplat create_starbunch ]
set myren [ ssplat create_renderer ]
ssplat load_tipsy_box ../data/test.tipsy $gas $stars $dark
$gas bunch_color { 1.0 0.0 0.0 }
$gas density 1.0
#$gas exponent_constant 100.0
$gas scale_length 0.1
$stars bunch_color { 0.0 1.0 0.0 }
$stars density 1.0
#$stars exponent_constant 100.0
$stars scale_length 0.1
$dark bunch_color { 0.0 0.0 1.0 }
$dark density 0.1
#$dark exponent_constant 100.0
$dark scale_length 0.1
$myren image_size 640 480
#$myren image_size 64 48
$myren camera $mycam
$myren debug on
$myren exposure_rescale_type log
$myren render test.tiff "$gas $stars $dark"
