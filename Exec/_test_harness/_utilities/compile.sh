#!/bin/bash
#from Nate's notes on slack:
#note - not _
export DISCHARGE_HOME=$HOME/_discharge/chombo-discharge
export CHOMBO_HOME=$DISCHARGE_HOME/Submodules/Chombo-3.3/lib
export OPT_SETTING=$1
export DEB_SETTING=$2
export DIM_SETTING=$3
export MPI_SETTING=$4
export EXAMPLE_DIRECTORY=$5
echo "compiling with options $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING"

module unload hdf5;
module load hdf5/parallel;
cd $CHOMBO_HOME        ; make -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING lib
cd $DISCHARGE_HOME     ; make -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING 
cd $EXAMPLE_DIRECTORY  ; $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING; make main -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING 
ls $EXAMPLE_DIRECTORY

module unload hdf5;
