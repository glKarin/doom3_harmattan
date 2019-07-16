#/bin/sh

#R="/usr/bin/doom.arm"
R="./doom3.arm"
HARM="+set h_sNoSound 3 +set h_sUsingAlsa 0 +set h_rNoLighting 1 +set h_cAlwaysRun 1 +set s_noSound 1"
COM="+set r_glDriver /usr/lib/libGLESv2.so +set r_fullScreen 1 +set com_purgeAll 0"
RENDERER="+set com_showFPS 1"

#DANTE="+set r_useDXT 0 +set r_useETC1 0 +set r_useETC1cache 1"

echo $R ${HARM} ${COM} ${RENDERER} $*

#gdb --args $R ${HARM} ${COM} ${RENDERER} $*

#$R ${HARM} ${COM} ${RENDERER} +set fs_game d3xp $*
$R ${HARM} ${COM} ${RENDERER} $*
