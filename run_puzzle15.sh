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
  export LD_LIBRARY_PATH=./lib/:$LD_LIBRARY_PATH 
  echo $LD_LIBRARY_PATH
  exec ./puzzle15 $*
fi

