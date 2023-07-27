#!/bin/bash
GCC=/usr/local/arm/arm-2009q3/bin/arm-none-linux-gnueabi-gcc 
if [ "${2}" == "thread" ];then
	thread=-pthread
else
	thread=""
fi

if [ "${1}" == "" ];then
	echo "缺少參數!"
	echo "請填入要交叉編譯的.c檔作為第一參數"
	echo "若有使用執行緒,請輸入thread作為第二參數"
else
	des=$1
	des="${des%.c}"
	[ $des == ${1} ] && echo "參數需為.c結尾" && exit -1
	${GCC} -static ${thread} ${1} -o ${des} && echo ${1} 完成交叉編譯！&& exit 0
fi


