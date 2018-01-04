#!/bin/bash

while getopts fw:h: option
do 
    case "$option" in
        f)
	    force_build="y";;
        w)
            w=$OPTARG;;
        h)
            h=$OPTARG;;
        \?)
            echo "Usage: args [-f] [-w width] [-h height]"
            echo "-f force to build"
            echo "-w font width"
            echo "-h font height"
            exit 1;;
    esac
done

if [ -z $DEVICE ];then
    exit
fi

cd ${DEVICE}

file=res/lang/zh-CN.bin
md5_file=res/lang/.md5
fmd5()
{
    find $file -type f | xargs md5sum > $md5_file
}

#-f Forced to build
if [ "$force_build" != "y" ]; then

    if [ ! -f $md5_file ]; then
    	fmd5
    else
        read old_md5 < $md5_file
    fi
    new_md5=($(find $file -type f | xargs md5sum))
    if [ "$old_md5" == "$new_md5" ]; then
        need_rebuild=0
    else
        echo "lang is changed"
        need_rebuild=1
    fi
    echo $new_md5 > $md5_file

    if [ $need_rebuild == 0 ]; then
        exit
    fi
fi

#default w
if [ "$w" == "" ]; then
    w=16
fi

#default h
if [ "$h" == "" ]; then
    h=16
fi

# generate font sxf_${width}x${height}.bin
# usage
# ./sxf_make font_file width height stdout>null stderr>null
#
./sxf_make arialuni.ttf $w $h 1>/dev/null 2>/dev/null

rm res/font/sxf.7z

# use 7zip to compress font file
# 7zip version: p7zip_9.20.1_src_all.tar.bz2
./7za a res/font/sxf.7z sxf_"$w"x"$h".bin


#delete the font file
rm sxf_"$w"x"$h".bin


cd -
