#!/usr/bin/python
from argparse import *
import os
import glob
import platform
from datetime import date
import signal

def signal_handler(signum, frame):
    raise Exception("Timed out!")

class class_status:
    def __init__(self,  a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi):
        self.dim_gmake  = " DIM=" + str(a_i_dim) + " "
        self.dim_status = "dim_"  + str(a_i_dim)
        if(a_i_opt == 0):
            self.opt_gmake  = " OPT=FALSE "
            self.opt_status =  "opt_false"
        else:
            self.opt_gmake  = " OPT=HIGH "
            self.opt_status =  "opt_high"
        if(a_i_deb == 0):
            self.deb_gmake  =  " DEBUG=FALSE "
            self.deb_status =   "debug_false"
        else:
            self.deb_gmake  = " DEBUG=TRUE "
            self.deb_status =  "debug_true"
        if(a_i_mpi == 0):
            self.mpi_status =  "mpi_false"
            self.mpi_gmake  = " MPI=FALSE "
        else:
            self.mpi_status =  "mpi_true"
            self.mpi_gmake  = " MPI=TRUE "
        self.all_status = self.opt_status + "_" + self.deb_status + "_" + self.dim_status + "_" + self.mpi_status
        self.exec_name  = a_example_prefix + "." + self.all_status + ".exe"
        print("leaving class_status constructor")

def compileStuff(  a_all_exec_dir_name,  a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi):
    print("inside compileStuff a_all_exec_dir_name = " + a_all_exec_dir_name)
    funk = class_status(a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi)
    all_status=     funk.all_status 
    opt_gmake =     funk.opt_gmake  
    deb_gmake =     funk.deb_gmake  
    dim_gmake =     funk.dim_gmake  
    mpi_gmake =     funk.mpi_gmake
    exec_name =     funk.exec_name
    print("inside compileStuff all_status = " + all_status)

    print("inside compileStuff for dim = " + str(a_i_dim) + ".  Compiled stuff goes to "  + a_all_exec_dir_name + "/" + exec_name)

    command_str = "cd " + a_driver_dir + "; ./compile.sh " + opt_gmake + deb_gmake + dim_gmake + mpi_gmake + "; mv main.exe " + a_all_exec_dir_name + "/" + exec_name + "; "
    print(command_str)
    os.system(command_str)
    
def setUpMPIRunDirectory(a_all_exec_dir_name, a_driver_dir, a_example_prefix, a_i_opt, a_i_deb, a_i_dim, a_i_mpi, a_i_num_proc):
    funk = class_status(a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi)
    all_status=     funk.all_status 
    opt_gmake =     funk.opt_gmake  
    deb_gmake =     funk.deb_gmake  
    dim_gmake =     funk.dim_gmake  
    mpi_gmake =     funk.mpi_gmake  
    exec_name =     funk.exec_name
    print("inside setUpMPIRunDirectory for dim = " + str(a_i_dim) + " and num procs = " + str(a_i_num_proc))
    print("inside setUpMPIRunDirectory all_status = " + all_status)
    
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

i_opt = 1
i_deb = 1
i_dim = 2
i_mpi = 1

#this could be done with iopera
driver_dir     = home_str + "/../../../../../Tests/Electrostatics/Profile"
example_prefix = "tests_profile"
while i_dim <= 3:
    comp_func = compileStuff(all_exec_dir_name, example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi)
    i_num_proc = 1
    while i_num_proc <= i_max_proc:
        setUpMPIRunDirectory(all_exec_dir_name, example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi, i_num_proc)
        i_num_proc = 2*i_num_proc
    i_dim = i_dim + 1

print_str = "Closing runall and exiting configure"
f_run_all.close()
print( print_str )





