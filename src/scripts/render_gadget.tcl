set mycam [ ssplat create_camera ]
$mycam from {700 0.0 700.0}
$mycam up {0.0 1.0 0.0}
$mycam hither 250.0
$mycam yon 1750.0
$mycam fovea 15.0
#$mycam projection parallel
set gas [ ssplat create_starbunch ]
set halo [ ssplat create_starbunch ]
set disk [ ssplat create_starbunch ]
set bulge [ ssplat create_starbunch ]
set stars [ ssplat create_starbunch ]
set bndry [ ssplat create_starbunch ]
if {[llength $argv] < 1} {
    error "You must supply a filename as an argument."
}
puts [ ssplat load_gadget $argv "$gas $halo $disk $bulge $stars $bndry" ] 
foreach thing { "$gas" "$halo" "$disk" "$bulge" "$stars" "$bndry" } {
    set npart [ eval $thing get nstars ]
    puts [ format "%%%% %s : %d particles" $thing $npart ]
    if [ expr $npart > 0 ] { eval $thing dump }
}
set myren [ ssplat create_renderer ]
$myren image_size 512 512
$myren camera $mycam
$myren debug 1
$myren exposure_rescale_type log
$myren log_rescale_bounds 1.0e-5 1.0
$myren exposure 0.5
$gas bunch_color { 0.8 0.5 1.0 0.1}
$gas density 0.0001
$gas exponent_constant 1
$halo bunch_color { 0.5 0.8 0.0 0.1}
$halo density 0.001
$halo exponent_constant 3
$disk bunch_color { 0.5 0.5 0.9 0.1}
$disk density 0.00001
$disk exponent_constant 3
$bulge bunch_color { 0.8 0.7 0.5 0.1}
$bulge density 0.00001
$bulge exponent_constant 1
$stars bunch_color { 1.0 1.0 1.0 0.3}
$stars density 0.001
$stars exponent_constant 10
$bndry bunch_color { 1.0 0.0 0.0 0.1}
$bndry density 0.003
$bndry exponent_constant 1
proc ren_points {} {
    global myren
    global gas halo disk bulge stars bndry
    $myren render_points test.tiff "$gas $halo $disk $bulge $stars $bndry"
}
proc ren_real {} {
    global myren
    global gas halo disk bulge stars bndry
#    $myren render test.tiff "$gas $halo $disk $bulge $stars $bndry"
    $myren render test.tiff "$gas $halo $disk $bulge $stars $bndry"
}

ren_real
#ren_points



