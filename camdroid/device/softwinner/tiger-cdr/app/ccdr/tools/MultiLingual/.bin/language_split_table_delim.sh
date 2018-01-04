#########################################################################
# File Name: language_split.sh
# Description: 
# Author: CaoDan
# mail: caodan@linuxtoy.cn
# Created Time: 2015年03月02日 星期一 15时26分16秒
#########################################################################
#!/bin/sh

input_file=$1
PWD=`pwd`
OUT_DIR=${PWD}/out
COLUMNS=0
LANGUAGE_FILE_LIST=()
LANGUAGE_FILE_COLUMN=()
LANG_LABEL_ID_COLUMN=1
LANG_LABEL_ID_FILE=${OUT_DIR}/lang_label.id
LANG_HEADER_FILE=${PWD}/../../include/misc/cdrLang.h

function Usage()
{
	echo -e "Invalid Arguments:"
	echo -e "Usage: "
	echo -e "$0 language_csv_file"
}

if [ $# -ne 1 ]; then
	Usage
	exit 1
fi

if [ ! -f $input_file ]; then
	echo "$input_file not exsist"
	Usage
	exit 1
fi

if [ ! -d $OUT_DIR ]; then
	mkdir -p $OUT_DIR
fi

######### convert input file encoding to UTF-8
flag=0
info=`file $input_file`

case $info in
	*UTF-8*)
		;;
	*UTF-16*)
		echo "convert encoding from UTF-16 to UTF-8"
		iconv -f UTF-16 -t UTF-8 $input_file > ${input_file}.tmp
		;;
	*)
		echo "invalid encoding"
		exit 1
esac

# remove ctrl symbols
input_file=${input_file}.tmp

case $info in
	*CRLF*)
		echo "remove ctrl lines"
		sed -i 's/\//g' $input_file
		;;
esac


############################ start #############################
COLUMNS=`head -n 1 $input_file | awk 'BEGIN{FS="\t"}; {print NF}'`
echo "find $COLUMNS columns"

######## get output file name
language_count=0
for i in `seq 1 $COLUMNS`
do
	str=`head -n 1 $input_file | awk 'BEGIN{FS="\t"}; {print $'"$i"'}'`
	if [ -n "$str" ]; then
		if [ "$str" = "LANG_LABEL_ID" ]; then
			LANG_LABEL_ID_COLUMN=$i
		else
			LANGUAGE_FILE_LIST[$language_count]=${OUT_DIR}/${str}.bin
			LANGUAGE_FILE_COLUMN[$language_count]=$i

			let language_count++
		fi
	fi
done

echo "total $language_count languages"
echo "LANG_LABEL_ID in column $LANG_LABEL_ID_COLUMN"

# split the language file
lines=`cat $input_file | wc -l`

# split LANG_LABEL_ID
tail -n $((lines - 2)) $input_file | cut -f $LANG_LABEL_ID_COLUMN > $LANG_LABEL_ID_FILE

for i in `seq 0 $(( ${#LANGUAGE_FILE_COLUMN[@]} - 1 ))`
do
	tail -n $((lines - 2)) $input_file | cut -f ${LANGUAGE_FILE_COLUMN[$i]} > ${LANGUAGE_FILE_LIST[ $i ]}
	echo -e "language $i: $str \t----->\t  `basename ${LANGUAGE_FILE_LIST[$i]}`"
done

# Parameter: file
function remove_last_empty_lines()
{
	local file=$1

	if [ -z $file ]; then
		return
	fi

	if [ ! -f $file ]; then
		echo "file $file not exsist"
		return
	fi

	while true
	do
		lines=`cat $file | wc -l`
		str=`tail -n 1 $file`
		if [ -z "$str" ]; then
			echo "the last line: $lines is empty"
			sed -i ''"$lines"' d' $file
		else 
			return
		fi
	done
}

# remove the last empty lines
for i in `seq 0 $((${#LANGUAGE_FILE_LIST[@]} - 1))`
do
	remove_last_empty_lines ${LANGUAGE_FILE_LIST[$i]}
done
remove_last_empty_lines $LANG_LABEL_ID_FILE

# remove the quotation
for i in `seq 0 $(( ${#LANGUAGE_FILE_LIST[@]} - 1 ))`
do
	sed -i '/^".*"$/ s/^"//g; s/"$//g '  ${LANGUAGE_FILE_LIST[$i]}
done

############# edit the LANG HEADER file

if [ ! -f $LANG_HEADER_FILE ]; then
	echo "can not find $LANG_HEADER_FILE"
	exit 1
fi

cp $LANG_HEADER_FILE ./
lang_header_file=`basename $LANG_HEADER_FILE`
start_line=`sed -n '/enum LANG_STRINGS/=' $lang_header_file`

##### remove the old enums
sed -i '/^enum LANG_STRINGS/,/^};/ d' $lang_header_file

##### add new enums
sed -i ' '"$start_line"' i\enum LANG_STRINGS {' $lang_header_file
let start_line++

lines=`cat $LANG_LABEL_ID_FILE | wc -l`
for i in `seq 1 $lines `
do
	str=`sed -n ' '"$i"' p ' $LANG_LABEL_ID_FILE`	
	if [ $i -eq 1 ]; then
		sed -i ' '"$start_line"' i\\t'"${str}"' = 0, \t //line '"$i"'' $lang_header_file
		#sed -i ' '"$start_line"' i\\t'"${str}"' = 0,' $lang_header_file
	else
		if [ $i -ne $lines ]; then
			sed -i ' '"$start_line"' i\\t'"${str}"', \t //line '"$i"'' $lang_header_file
			#sed -i ' '"$start_line"' i\\t'"${str}"',' $lang_header_file
		else
			sed -i ' '"$start_line"' i\\t'"${str}"' \t //line '"$i"'' $lang_header_file
		fi
	fi
	let start_line++
done

sed -i ' '"$start_line"' i\};' $lang_header_file

mv $lang_header_file $LANG_HEADER_FILE

exit 0




