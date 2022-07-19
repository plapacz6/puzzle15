#author: plapacz6@gmail.com
#data: 2020-07-16
#ver: 0.1.0

INCLUDE_DIR=/usr/local/include/opencv4/

LIB_DIR=/usr/local/lib/
#LIB_DIR=./lib/
#c++1z -> c++17
CXXC=g++ -std=c++1z 
CC=gcc
CFLAGS= -Wall -g 
LFLAGS= 
LIBS_0=-lopencv_calib3d \
-lopencv_core \
-lopencv_dnn \
-lopencv_features2d \
-lopencv_flann \
-lopencv_gapi \
-lopencv_highgui \
-lopencv_imgcodecs \
-lopencv_imgproc \
-lopencv_ml \
-lopencv_objdetect \
-lopencv_photo \
-lopencv_stitching \
-lopencv_videoio \
-lopencv_video \
-ldl
	
PUZZLE15_LIB=-lopencv_core \
-lopencv_highgui \
-lopencv_imgcodecs \
-lopencv_imgproc \

# puzzle15l: puzzle15.cc
# 	#./export_lib_path.sh  #for application run
# 	#g++ -g --std=c++1z $< -I$(INCLUDE_DIR) -L$(LIB_DIR) `pkg-config --libs opencv` -o $@
# 	g++ -g --std=c++1z $< -I$(INCLUDE_DIR) -L$(LIB_DIR) $(PUZZLE15_LIB) -o $@
	
puzzle15: puzzle15.o	
	g++ -g -no-pie --std=c++1z $< -L$(LIB_DIR) $(PUZZLE15_LIB) -o $@

puzzle15.o: puzzle15.cc	
	g++ -g -no-pie --std=c++1z $< -I$(INCLUDE_DIR) -c

.PHONY: clean

clean:
	- rm *.o	
	for i in `ls *.cpp`; do printf "deleting: %s \n" $${i%.cpp}; rm $${i%.cpp}; done;
	for i in `ls *.cc`; do printf "deleting: %s \n" $${i%.cc}; rm $${i%.cc}; done;
	for i in `ls *.cxx`; do printf "deleting: %s \n" $${i%.cxx}; rm $${i%.cxx}; done;
	for i in `ls *.c`; do printf "deleting: %s \n" $${i%.c}; rm $${i%.c}; done;
  
