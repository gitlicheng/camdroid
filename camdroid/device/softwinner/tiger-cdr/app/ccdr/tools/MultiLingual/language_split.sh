#########################################################################
# File Name: language_split.sh
# Description: 
# Author: CaoDan
# mail: caodan@linuxtoy.cn
# Created Time: 2015年03月02日 星期一 17时19分36秒
#########################################################################
#!/bin/sh

BIN_PATH="`pwd`/.bin"

function Usage()
{
	echo -e "Invalid Arguments:"
	echo -e "Usage: "
	echo -e "$0 input_file"
}

if [ $# -ne 1 ]; then
	Usage
	exit 1
fi

echo -e "select delimiter:"
echo -e "\t 1. table (Unicode txt convert from excel)"
echo -e "\t 2. comma (csv file convert from excel)"

read val
if [ ${val}x = "x" ]; then
	echo "bad input choice"
	exit 1
fi

case $val in  
	1)
		echo "select Unicode txt"
		$BIN_PATH/language_split_table_delim.sh $1
		;;
	2)
		echo "select csv file"
		$BIN_PATH/language_split_csv_delim.sh $1
		;;
	*)
		echo "bad input choice"
		;;	
esac
