#! /bin/bash
#用法：
#从当前目录递归生成文件：sh code_gen.sh
#从当前目录递归删除文件：sh code_gen.sh rm


tpl_file_ext='tpl'
current_dir=`pwd`
datamodel_dir='../../datamodel/model/'
line=2 # which line is xml_file path, ref to datamodel_dir

#param1:direction     param2: pump/rm
function read_dir(){
    for file in `ls $1`
    do
        if [ -d $1"/"$file ]
        then
            read_dir $1"/"$file $2
        else

            filename=$(echo $file | sed 's/\.[^.]*$//')
            extension=$(echo $file | sed 's/^.*\.//')
            if [ $extension == $tpl_file_ext ]; then
                target_file=$1"/"$filename
                tpl_file=$1"/"$file

                second_line=`sed -n '2p' ${tpl_file}`
                second_line=`echo ${second_line}|sed 's/<!--//g' |sed 's/-->//g'|sed 's/@depend//g'|sed 's/\/\/\///g'|sed 's/\r//g'` #rm head or tail space
                second_line=`echo ${second_line}` #rm head or tail space
                xml_file=${datamodel_dir}${second_line}
                if [ ! -f "${xml_file}" ];then
                    echo " file not exist : ${xml_file} "
                fi

#                xml_file="/home/wxuser/Desktop/utrade/hello/pump_for_linux/datamodel/commonEnv/UFDataType.xml"

                if [ "$2" == "pump" ]; then
                    pump $target_file $tpl_file $xml_file
                    echo $tpl_file + $xml_file = $target_file
                else
                    rm -f $target_file
                    echo rm -f $target_file
                fi
#                sleep 1
            fi
        fi
    done
}

echo -e "\nUTrade Start Generate Code"

param1=$current_dir
param2="pump"

if [ $# -gt 0 ] ;then
    if [ "$1" == "rm" ] ;then
        param2="rm"
    fi
fi

read_dir $param1 $param2
echo -e "UTrade Complete Generate Code\n"
