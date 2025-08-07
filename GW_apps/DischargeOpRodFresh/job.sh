#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=00:30:00
#SBATCH --nodes=4
#SBATCH --ntasks-per-node=64
#SBATCH --constraint=cpu

srun -n 256 ./program3d.Linux.64.CC.ftn.OPTHIGH.MPI.ex flatflat.inputs

