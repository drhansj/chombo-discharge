#!/bin/bash
#SBATCH --qos=debug
#SBATCH --time=00:30:00
#SBATCH --nodes=5
#SBATCH --ntasks-per-node=128
#SBATCH --constraint=cpu

srun -n 40 ./program3d.Linux.64.CC.ftn.OPTHIGH.MPI.ex example.inputs

