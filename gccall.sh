#! /bin/sh

BASEDIR=$(dirname "$0")
gcc $BASEDIR/OSMP/OSMPExecutable/osmpexecutable.c -o $BASEDIR/OSMP/OSMPExecutable/osmpexecutable
gcc $BASEDIR/OSMP/OSMPStarter/osmprun.c -o $BASEDIR/OSMP/OSMPStarter/osmprun
gcc $BASEDIR/Lib/Echoall/echoall.c -o $BASEDIR/Lib/Echoall/echoall