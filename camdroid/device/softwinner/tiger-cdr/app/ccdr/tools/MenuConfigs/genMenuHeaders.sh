#########################################################################
# File Name: generateMenuHeaders.sh
# Description: 
# Author: CaoDan
# mail: caodan@linuxtoy.cn
# Created Time: 2015年03月13日 星期五 14时03分57秒
#########################################################################
#!/bin/sh

PWD=`pwd`
CMDDIR=`dirname $0`
subdirs=()
outdir=out
autogenScript="./.autogenMenu_h.sh"

cd $CMDDIR
for str in `ls -d */`
do
	str=${str%/*}
	file="${str}/${str}.txt"
	if [ -f "$str/$str.txt" ]; then
		subdirs=(${subdirs[@]} $str)
	fi
done

if [ ${#subdirs[@]} -eq 0 ]; then
	echo "not find config Files"
	cd $PWD
	exit 1
fi

echo "find ${#subdirs[@]} menu config files"
for i in `seq 0 $(( ${#subdirs[@]} - 1 ))`
do
	echo -e "\t $i ${subdirs[$i]}"
done

function genHeaders()
{
	local inputFile
	local outputFile

	if [ ! -d "$outdir" ]; then
		mkdir $outdir	
	fi

	for i in `seq 0 $(( ${#subdirs[@]} - 1 ))`
	do
		inputFile="${subdirs[$i]}/${subdirs[$i]}.txt"
		outputFile="$outdir/Menu${subdirs[$i]}.h"
		$autogenScript $inputFile $outputFile
		echo -e " ... ... ... ... ... ... ... ... ... generate $outputFile"
	done
}

echo -n "generate Menu Header files? (y/Y/n/N): "
read value
case $value in 
	[yY])
		genHeaders
		;;
	[nN])
		;;
	*)
		;;
esac

cd $PWD
exit 0



