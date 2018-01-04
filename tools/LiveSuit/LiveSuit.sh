#!/bin/bash
set  -e

if [ -L $0 ]; then
	AbsPath=`readlink $0`
else
	AbsPath=$0
fi

AppName=`basename $AbsPath | sed s,\.sh$,,`
AppDir=bin

BinDir=`dirname $AbsPath`
tmp="${BinDir#?}"

if [ "${BinDir%$tmp}" != "/" ]; then
	BinDir=$PWD/$BinDir
fi

BinDir=$BinDir/$AppDir

LD_LIBRARY_PATH=$BinDir
export LD_LIBRARY_PATH

os=`cat /etc/issue | head -n 1 | cut -d\  -f1`
echo "You are running on" $os

$BinDir/$AppName "$@" >/dev/null
