#!/bin/sh

make clean               2> /dev/null
make distclean           2> /dev/null
rm -rf autom4te.cache     2> /dev/null
rm ./configyre           2> /dev/null
rm ./acinclude.m4        2> /dev/null
rm ./configure           2> /dev/null 
rm ./intltool-extract    2> /dev/null
rm ./intltool-merge      2> /dev/null
rm ./intltool-update     2> /dev/null
rm ./stamp-h ./stamp-h.in     2> /dev/null
