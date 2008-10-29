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

ext set association -open "xpdf %fn" {\.pdf$}
ext set association -edit "mcedit %fn" -open "emacs %fn" {\.txt$}
ext set association -edit "vim %fn" -open "vim %fn" {\.pl$}
ext set association -edit "gvim %fn" -open "gvim %fn" {\.tcl$}
ext set association -open "feh %fn" -edit "gimp %fn" {\.jpg|gif|png$}
ext set association -open "mplayer %fn" {\.mp3|ogg|flac|avi$}
