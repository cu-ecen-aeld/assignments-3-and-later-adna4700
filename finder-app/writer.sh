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

dirnm=$( dirname $1 )
mkdir -p ${dirnm}

#Test for directory check
if [ -d ${dirnm} ]
then
	echo "The ${writefile} found"
	echo ${writestr} > ${writefile}
	exit 0
else

	echo "${writefile} not found; will be created"
	exit 1
	
fi



