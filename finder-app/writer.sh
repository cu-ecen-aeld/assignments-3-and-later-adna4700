#!/bin/bash
writefile=$1
writestr=$2

#Testing for number of arguments
if [ $# -eq 2 ]
then
	echo "Number of arguments meet the requirements"
else
	echo "failed: number of parameters should be two.
	      1. writefile - file location to write the string
	      2. writestr- string to be written"
	exit 1
fi

dirnm= dirname $1
mkdir ${dirnm}
touch $1

#Test for directory check
if [ -d $1 ]
then
	echo "The ${writefile} found"
	${writestr} > ${writefile}
else

	echo "${writefile} cannot be created"
	exit 1
	
fi


#${writestr} > ${writefile}

