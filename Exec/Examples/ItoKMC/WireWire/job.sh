#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=00:30:00
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=16
#SBATCH --constraint=cpu

srun -n 16 ./program3d.Linux.64.CC.ftn.OPTHIGH.MPI.ex example.inputs

