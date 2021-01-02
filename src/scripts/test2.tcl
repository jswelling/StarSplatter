sset mycam [ ssplat create_camera ]
$mycam from { 0.0 0.0 3.0 }
$mycam at { 0.0 0.0 0.0 }
$mycam up { 0.0 1.0 0.0 }
$mycam hither 1.0
$mycam yon 5.0
set gas [ ssplat create_starbunch ]
set stars [ ssplat create_starbunch ]
set dark [ ssplat create_starbunch ]
ssplat load_tipsy_box ../data/test.tipsy $gas $stars $dark
$gas bunch_color { 1.0 0.0 0.0 0.3 }
$gas density 0.0003
$gas scale_length 0.02
$stars bunch_color { 0.0 1.0 0.0 0.3 }
$stars density 0.0003
$stars scale_length 0.02
$dark bunch_color { 0.0 0.0 1.0 0.3 }
$dark density 0.0003
$dark scale_length 0.02
set myren [ ssplat create_renderer ]
$myren image_size 640 480
$myren camera $mycam
$myren debug on
$myren render test.tiff "$gas $stars $dark"

