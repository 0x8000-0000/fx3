#Set this to @ to keep the makefile quiet
#SILENCE = @

#---- Outputs ----#
COMPONENT_NAME=fx3
TARGET_LIB=lib/lib$(COMPONENT_NAME).a

TEST_TARGET=$(COMPONENT_NAME)_tests

#--- Inputs ----#
CPP_PLATFORM=Gcc

#CFLAGS are set to override malloc and free to get memory leak detection in C programs
CPPUTEST_CFLAGS+=-std=c11
#GCOVFLAGS = -fprofile-arcs -ftest-coverage
CPPUTEST_USE_MEM_LEAK_DETECTION=N

#SRC_DIRS is a list of source directories that make up the target library
#If test files are in these directories, their IMPORT_TEST_GROUPs need
#to be included in main to force them to be linked in.  By convention
#put them into an AllTests.h file in each directory
SRC_DIRS=src ../arch/portable

#TEST_SRC_DIRS is a list of directories including 
# - A test main (AllTests.cpp by conventin)
# - OBJ files in these directories are included in the TEST_TARGET
# - Consequently - AllTests.h containing the IMPORT_TEST_GROUPS is not needed
TEST_SRC_DIRS=test

#includes for all compiles	
INCLUDES=-Iinc -I$(CPPUTEST_HOME)/include -I../arch/inc -Iconfig

include $(CPPUTEST_HOME)/build/MakefileWorker.mk

