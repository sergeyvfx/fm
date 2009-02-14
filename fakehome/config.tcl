
# ${project-name} - a GNU/Linux console-based file manager
#
# Main configuration file
#
# Copyright 2008 Sergey I. Sharybin <g.ulairi@gmail.com>
# Copyright 2008 Alex A. Smirnov <sceptic13@gmail.com>
#
# This program can be distributed under the terms of the GNU GPL.
# See the file COPYING.
#

#
# Edit this file on your own risk
#

set escdelay 2000

::config::bind . <C-x><C-f> { ::actions::find }
# ::config::bind . <F1> {
#     ::iface::message_box -title "Exit" -message "A u ready?" -type yesno
# }

# ::config::bind . <C-x><C-c> { ::core::exit }

# ::config::bind <<edit-class-context>> <C-x> {
#     ::iface::message_box -title "Message" -message "test" -type yesno
# }

# ::config::bind . <M-f> { ::core::exit }

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
