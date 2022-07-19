#!/bin/sh
if [ -z $1 ] 
then
  printf "\n"
  printf  "\tusage: ./run_puzzle15 image.jpg 25\n"
  printf  "\timage.jpg - image to solve\n"
  printf  "\tdifficult  : difficulty level 2-50 (defalt 15)\n"
  printf "\n"
else
  echo  $0 $1 $2
  export LD_LIBRARY_PATH=/usr/local/lib:./lib/:$LD_LIBRARY_PATH 
  ./puzzle15 $*
fi

