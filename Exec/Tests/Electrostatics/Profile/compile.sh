#!/bin/bash
#from Nate's notes on slack:
#note - not _
export DISCHARGE_HOME=$HOME/_discharge/chombo-discharge
export CHOMBO_HOME=$DISCHARGE_HOME/Submodules/Chombo-3.3/lib
module unload hdf5;
module load hdf5/parallel;
cd $CHOMBO_HOME   ; make -j32 MPI=TRUE DEBUG=FALSE DIM=3 lib
cd $DISCHARGE_HOME; make -j32 MPI=TRUE DEBUG=FALSE DIM=3
cd $DISCHARGE_HOME/Exec/Tests/Electrostatics/Profile; make -j32 MPI=TRUE DEBUG=FALSE DIM=3
module unload hdf5;
