#!/bin/bash
#SBATCH --qos=regular
#SBATCH --time=03:30:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=128
#SBATCH --constraint=cpu

srun -n 128 ./program3d.Linux.64.CC.ftn.OPTHIGH.MPI.ex opRod_v4-Working.inputs

