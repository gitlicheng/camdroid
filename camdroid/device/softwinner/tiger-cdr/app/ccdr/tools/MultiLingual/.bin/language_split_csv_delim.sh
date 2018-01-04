#########################################################################
# File Name: language_split.sh
# Author: CaoDan
# mail: caodan2519@gmail.com
# Created Time: 2014年11月17日 星期一 15时19分59秒
#########################################################################
#!/bin/sh

input_file=$1
out_dir=out
LANGUAGE_FILE_LIST=()

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

if [ ! -d $out_dir ]; then
	mkdir -p $out_dir
fi

######### convert input file encoding to UTF-8
file $input_file | grep UTF-8
if [ $? -ne 0 ]; then
	echo "need UTF-8 Encoding file"
	iconv -f gb18030 -t utf8 $input_file > ${input_file}.tmp
	input_file=${input_file}.tmp
fi

language_count=`head -n 1 $input_file | awk -F ',' '{print NF}'`
echo "language_count is $language_count"

######## get output file name
count=0
for i in `seq 1 $language_count`
do
	str=`head -n 1 $input_file |	cut -d "," -f $i`
	if [ -n "$str" ]; then
		echo "$str" | grep -P -q "\S+\s+\S+"
		if [ $? -eq 0 ]; then
			str=`echo $str | cut -d " " -f 2`
		fi
		str=${out_dir}/${str}.bin
		LANGUAGE_FILE_LIST[$i]=$str

		echo -e "language $i: $str \t----->\t  output file ${LANGUAGE_FILE_LIST[$i]}"
		let count++
	else
		break;
	fi
done

language_count=$count
echo "total $language_count languages"

################## split the csv in to language bin files
for i in `seq 1 $language_count`
do
	file=${LANGUAGE_FILE_LIST[$i]}
	awk 'BEGIN\
	{ \
		FS=","; NF='"$language_count"'; count=0;\
	}\
	{\
		c=0;\
		for(n=1; n<=NF; n++) {\
			if(q==1){ \
				if($n~/"$/) { q=0 } \
				$c = $c FS $n; continue \
			} \
			if($n~/^"/) { q=1; $(++c) = $n; continue } \
			$(++c) = $n \
		}\
	} \
	{\
		count++; if(count>2)print $'"$i"'; \
	}' $input_file 1> ${LANGUAGE_FILE_LIST[$i]}

done

# remove the last empty lines
for i in `seq 1 $language_count`
do
	while true
	do
		lines=`wc -l ${LANGUAGE_FILE_LIST[$i]} | cut -d " " -f 1`
		str=`sed -n ' '"$lines"' p' ${LANGUAGE_FILE_LIST[$i]}`
		if [ -z "$str" ]; then
			echo "line $lines is empty"
			sed -i ''"$lines"' d' ${LANGUAGE_FILE_LIST[$i]}
		else 
			break;
		fi
	done
done

# remove the quotation
for i in `seq 1 $language_count`
do
	sed -i '/^".*"$/ s/^"//g; s/"$//g '  ${LANGUAGE_FILE_LIST[$i]}
done


exit 0

