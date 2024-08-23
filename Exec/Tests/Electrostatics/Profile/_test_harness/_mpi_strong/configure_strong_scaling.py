#!/usr/bin/python
from argparse import *
import os
import glob
import platform
from datetime import date
import signal

def signal_handler(signum, frame):
    raise Exception("Timed out!")

def compileStuff(a_i_dim, a_all_exec_dir_name, a_driver_dir):
    #this stuff could be sent in but for this test these are always true
    opt_gmake  = " OPT=HIGH "
    deb_gmake  = " DEBUG=TRUE "
    dim_gmake  = " DIM=" + str(i_dim) + " "
    mpi_gmake  = " MPI=TRUE "
    opt_status = "opt_high"
    deb_status = "debug_true"
    dim_status = "dim_"  + str(i_dim)
    mpi_status = "mpi_true"
    all_status = opt_status + "_" + deb_status + "_" + dim_status + "_" + mpi_status
    exec_name  = "main." + all_status + ".exe"
    print("inside compileStuff for dim = " + str(a_i_dim) + ".  Compiled stuff goes to "  + a_all_exec_dir_name + "/" + exec_name)

    command_str = "cd " + a_driver_dir + "; ./compile.sh " + opt_gmake + deb_gmake + dim_gmake + mpi_gmake + "; mv main.exe " + a_all_exec_dir_name + "/" + exec_name + "; "
    print(command_str)
    os.system(command_str)
    
def setUpMPIRunDirectory(a_i_dim, a_i_num_proc):
    #this stuff could be sent in but for this test these are always true
    opt_status = "opt_high"
    deb_status = "debug_true"
    dim_status = "dim_"  + str(i_dim)
    mpi_status  = "mpi_true"
    all_status = opt_status + "_" + deb_status + "_" + dim_status + "_" + mpi_status
    print("inside setUpMPIRunDirectory for dim = " + str(a_i_dim) + " and num procs = " + str(a_i_num_proc))
    
today = date.today()

print("Today's date:", today)

parser = ArgumentParser()

parser.add_argument('--batch', type=str, help='batch file template'   ,default="../_batch_templates/spencer.batch")
parser.add_argument('--input', type=str, help='batch file template'   ,default="../_input_templates/circle.input.template")
parser.add_argument('--max_num_proc', type=int, help='max number of processors for each run'   ,default='8')
parser.add_argument('--prefix', type=str, help='name of test["mpi_strong"]',default="mpi_strong")

args = parser.parse_args()
print(args)
home_str = os.getcwd();
print ("homedir = " + home_str)
neartop_directory = home_str + "/_" + args.prefix
strtoday =str(today.month) + "_" + str(today.day) + "_" + str(today.year)
top_directory = neartop_directory + "_" + strtoday
print ("top_directory = " + top_directory)
if not os.path.exists(top_directory):
    printstr = "making directory " + top_directory
    print (printstr)
    os.mkdir(top_directory)
all_exec_dir_name = top_directory + "/_executable_files"
if not os.path.exists(all_exec_dir_name):
    printstr = "making directory " + all_exec_dir_name
    print (printstr)
    os.mkdir(all_exec_dir_name)
batch_template = home_str + "/" + args.batch
run_all_file_name = top_directory + "/run_all.sh"
f_run_all       = open( run_all_file_name,'w')
i_max_proc = args.max_num_proc

f_run_all.write('#/usr/bin/csh\n')

i_dim = 2
while i_dim <= 3:
    driver_dir = home_str + "/../../../Profile"
    compileStuff(i_dim, all_exec_dir_name, driver_dir)
    i_num_proc = 1
    while i_num_proc <= i_max_proc:
        setUpMPIRunDirectory(i_dim, i_num_proc)
        i_num_proc = 2*i_num_proc
    i_dim = i_dim + 1

print_str = "Closing runall and exiting configure"
f_run_all.close()
print( print_str )





