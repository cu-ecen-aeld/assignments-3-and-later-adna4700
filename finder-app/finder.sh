#!/bin/bash
filesdir=$1
searchstr=$2

#Testing for number of arguments
if [ $# -eq 2 ]
then	
        echo "Number of arguments meet the requirements"	
else
	echo "failed: number of parameters should be two. 
	1. filedir - Path to the directory to search strings in 
	2. searchstr- string to be searched"
	exit 1
fi

#Test for directory check
if [ -d $1 ]
then 
	echo "The ${filesdir} found"
else
	echo "The ${filesdir} not found on the filesystem"
	exit 1
fi

#Enter the directory to search the string
cd $1

#Reference:  https://unix.stackexchange.com/questions/88040/how-to-use-the-grep-result-in-command-line#:~:text=%24%20grep%20-lr%20%27%20Linux%27%20%2A%20path0%2Foutput.txt%20path1%2Foutput1.txt,on%20the%20command%20line%20when%20passed%20to%20vim%3A
matchfiles=$( grep -lr "${searchstr}" * | wc -l )

#grep -r "${searchstr}" ${filesdir} > grep.txt
#matchlines= cat grep.txt | wc -l
matchlines=$( grep -r "${searchstr}" * | wc -l )

echo "The number of files are ${matchfiles} and the number of matching lines are ${matchlines}"

exit 0

