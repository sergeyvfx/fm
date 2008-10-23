set escdelay 2000

#::iface::message_box -title "Это просто Title" \
#    -message "Используй Силу!" -type yesno

#::iface::message_box -title "Fm eating your mind" \
#    -message "Mva-ha-ha-ha-ha" -critical
#
if 0 {
if {[::iface::message_box -title "Exit" \
			  -message "A u ready?" -type yesno] == yes} {
    ::core::exit
}
}
# vim:ts=8:sw=4:sts=4:noet
