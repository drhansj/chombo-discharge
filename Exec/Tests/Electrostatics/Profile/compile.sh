#!/bin/bash
#from Nate's notes on slack:
# I think this will work:
# compile.sh FALSE TRUE 2 FALSE for gdb'able 2D

export DISCHARGE_HOME=$HOME/_discharge/chombo-discharge
export CHOMBO_HOME=$DISCHARGE_HOME/Submodules/Chombo-3.3/lib
export OPT_SETTING=$1
export DEB_SETTING=$2
export DIM_SETTING=$3
export MPI_SETTING=$4
export COM_OPTIONS="OPT=$OPT_SETTING DEBUG=$DEB_SETTING DIM=$DIM_SETTING MPI=$MPI_SETTING"

echo "compile.sh: compiling with the following settings: " 
echo "compile.sh: deb             = " $DEB_SETTING
echo "compile.sh: dim             = " $DIM_SETTING
echo "compile.sh: mpi             = " $MPI_SETTING
echo "compile.sh: compile_options = " $COM_OPTIONS

if [ $MPI_SETTING == "TRUE" ]
then
    echo "compile.sh: changing hdf5 and petsc to parallel";
    module unload hdf5 ; module load hdf5/parallel;
    module unload petsc; module load petsc/parallel;
else
    echo "compile.sh: changing hdf5 and petsc to serial";
    module unload hdf5 ; module load hdf5/serial;
    module unload petsc; module load petsc/serial;
fi
echo "make realclean" 
make realclean;

echo "compile.sh:cd $CHOMBO_HOME ; make -j32 lib"
cd $CHOMBO_HOME; make -j32 $COM_OPTIONS lib;
echo "compile.sh:cd $DISCHARGE_HOME; make -j32 $COM_OPTIONS" all;
cd $DISCHARGE_HOME; make -j32 $COM_OPTIONS;
echo "compile.sh:cd $DISCHARGE_HOME/Exec/Tests/Electrostatics/Profile; make -j32 $COM_OPTIONS all";
cd $DISCHARGE_HOME/Exec/Tests/Electrostatics/Profile; make -j32 $COM_OPTIONS all;
echo "compile.sh:mv program*.ex main.exe"
mv program*.ex main.exe

