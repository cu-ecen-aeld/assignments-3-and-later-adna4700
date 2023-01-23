#!/bin/bash
filesdir=$1
searchstr=$2

#Testing for number of arguments
if [ $# -eq 2 ]
then	
       echo "Number of arguments meet the requirements"	
else
	echo "failed: number of parameters should be two. 
	1. filedir - file location to search strings in 
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

matchfiles= ls | wc -l
cd $1
#grep -r "${searchstr}" ${filesdir} > grep.txt
#matchlines= cat grep.txt | wc -l
matchlines= grep -r "${searchstr}" * | wc -l

echo "The number of files are ${matchfiles} and the number of matching lines are ${matchlines}"

