#!/bin/sh
umask 022
set -e
make clean all CONFIGURATION=Release
make clean all CONFIGURATION=Debug
make clean all CONFIGURATION=Debug SDK=macosx10.5
make clean

