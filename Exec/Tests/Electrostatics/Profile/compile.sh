#!/bin/bash
#from Nate's notes on slack:
#note - not _
export DISCHARGE_HOME=$HOME/_discharge/chombo-discharge
export CHOMBO_HOME=$DISCHARGE_HOME/Submodules/Chombo-3.3/lib
export OPT_SETTING=$1
export DEB_SETTING=$2
export DIM_SETTING=$3
export MPI_SETTING=$4
echo "compiling with options $COMPILE_OPTIONS"

module unload hdf5;
module load hdf5/parallel;
\rm *.ex
cd $CHOMBO_HOME                                     ; make -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING lib
cd $DISCHARGE_HOME                                  ; make -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING 
cd $DISCHARGE_HOME/Exec/Tests/Electrostatics/Profile; make -j32 $OPT_SETTING $DEB_SETTING $DIM_SETTING $MPI_SETTING
mv program*.ex main.exe
module unload hdf5;
