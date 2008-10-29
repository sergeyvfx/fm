#
# ${project-name} - a GNU/Linux console-based file manager
#
# File associations configuration file
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

#
# Variables:
#   %n   - name of file inside current working directory
#   %cwd - current working directory
#   %fn  - full (absolutely) file name
#

ext set association -open "xpdf %n" {\.pdf$}
ext set association -edit "mcedit %n" -open "emacs %n" {\.txt$}
ext set association -edit "vim %n" -open "vim %n" {\.pl$}
ext set association -edit "gvim %n" -open "gvim %n" {\.tcl$}
ext set association -open "feh %n" -edit "gimp %n" {\.jpg|gif|png$}
ext set association -open "mplayer %n" {\.mp3|ogg|flac|avi$}
