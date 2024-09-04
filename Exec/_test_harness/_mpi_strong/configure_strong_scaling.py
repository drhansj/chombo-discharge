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
        self.example_prefix = a_example_prefix
        
        self.all_status = self.example_prefix + "_" + self.opt_status + "_" + self.deb_status + "_" + self.dim_status + "_" + self.mpi_status
        self.exec_name  = a_example_prefix + "." + self.all_status + ".exe"
        #print("leaving class_status constructor")

def compileStuff(  a_all_exec_dir_name,  a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi):
    #print("inside compileStuff a_all_exec_dir_name = " + a_all_exec_dir_name)
    funk = class_status(a_example_prefix, a_driver_dir, a_i_opt, a_i_deb, a_i_dim, a_i_mpi)

    all_status=     funk.all_status 
    opt_gmake =     funk.opt_gmake  
    deb_gmake =     funk.deb_gmake  
    dim_gmake =     funk.dim_gmake  
    mpi_gmake =     funk.mpi_gmake
    exec_name =     funk.exec_name
    #print("inside compileStuff all_status = " + all_status)
    print("inside compileStuff for dim = " + str(a_i_dim) + ".  Compiled stuff goes to "  + a_all_exec_dir_name + "/" + exec_name)
    compile_script = home_str + "/../_utilities/compile.sh"
    command_str =  compile_script +  " " +opt_gmake + deb_gmake + dim_gmake + mpi_gmake + driver_dir + "; mv " + driver_dir + "/main.exe " + a_all_exec_dir_name + "/" + exec_name + "; "
    print(command_str)
    os.system(command_str)
    
    
today = date.today()

print("Today's date:", today)

parser = ArgumentParser()

parser.add_argument('--batch', type=str, help='batch file template'   ,default="../_batch_templates/spencer.batch")
parser.add_argument('--max_num_proc', type=int, help='max number of processors for each run'   ,default='8')
parser.add_argument('--prefix', type=str, help='name of test["mpi_strong"]',default="mpi_strong")
parser.add_argument('--max_lev_min', type=int, help='minimum testing max level (0)',default='0')
parser.add_argument('--max_lev_max', type=int, help='maximum testing max level (8)',default='8')
parser.add_argument('--sbatch_instead_of_source', type=bool, help='Whether run.sh calls sbatch instead of source for each case[False].' ,default=False)


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


i_opt = 1
i_deb = 1
i_dim = 2
i_mpi = 1

batch_template = home_str + "/" + args.batch
run_all_file_name = top_directory + "/run_all.sh"
f_run_all       = open( run_all_file_name,'w')
f_run_all.write('#/usr/bin/csh\n')
batch_root = "batch_4586.sh"
input_root = "4586.inputs"

#This is just an ordered list of directory/input file/prefix/dim combinations
i_num_example = 4
i_cur_example = 0
while (i_cur_example < i_num_example):

    driver_dir     = "4586"
    input_template = "4586"
    i_dim          =  4586
    i_found        =  0
    
    if(i_cur_example == 0):
        driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
        input_template = home_str + "/../_input_templates/tests_profile_circle.input.template"
        i_dim          = 2
        example_prefix = "tests_profile_circle"
        i_found = 1
    elif(i_cur_example == 1):
        driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
        input_template = home_str + "/../_input_templates/tests_profile_square.input.template"
        i_dim          = 2
        example_prefix = "tests_profile_square"
        i_found = 1
    elif(i_cur_example == 2):
        driver_dir     = home_str + "/../../Tests/AdvectionDiffusion/Godunov"
        input_template = home_str + "/../_input_templates/tests_advdiff_godunov_2d.input.template"
        i_dim          = 2
        example_prefix = "tests_profile_advdiff_godunov_2d"
        i_found = 1
    elif(i_cur_example == 3):
        driver_dir     = home_str + "/../../Tests/AdvectionDiffusion/Godunov"
        input_template = home_str + "/../_input_templates/tests_advdiff_godunov_3d.input.template"
        i_dim          = 2
        example_prefix = "tests_profile_advdiff_godunov_3d"
        i_found = 1

    if(i_found == 0):
        print("configure_strong_scaling: logic error")
        exit()

    print("driver_dir    [" + str(i_cur_example)  +  "] = " + driver_dir)
    print("input_template[" + str(i_cur_example)  +  "] = " + input_template)
    print("input_template[" + str(i_cur_example)  +  "] = " + input_template)
    print("i_dim         [" + str(i_cur_example)  +  "] = " + str(i_dim))
    print("example_prefix[" + str(i_cur_example)  +  "] = " + example_prefix)

    #compile executables and cleverly name and cache them
    funk      =                    class_status(example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi)
    comp_func = compileStuff(all_exec_dir_name, example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi)
    i_max_lev = args.max_lev_min
    while i_max_lev <= args.max_lev_max:
        runs_directory = top_directory + "/_max_lev" + str(i_max_lev) + "_" + funk.all_status + "_runs"
        if not os.path.exists(runs_directory):
            print("making directory " + runs_directory)
            os.mkdir(runs_directory)
    
        i_num_proc = 1
        while i_num_proc <= args.max_num_proc:
            mpi_directory = runs_directory + "/_" + str(i_num_proc)  + "_procs" 
            if not os.path.exists(mpi_directory):
                print("making directory " + mpi_directory)
                os.mkdir(mpi_directory)

            funk = class_status(example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi)
            full_exec_name = all_exec_dir_name + "/" + funk.exec_name
            print("full_exec_name = " + full_exec_name) 
            print("all_exec_dir_name = " + all_exec_dir_name) 
            #soft link executable to rundir_name/main.exe
            main_name = mpi_directory + "/main.exe"
            if(os.path.exists(main_name)):
                command_str = "rm " + main_name
                print(    command_str)
                os.system(command_str)
                
            command_str =  " ln -s "   + full_exec_name + " " + main_name
            print(    command_str)
            os.system(command_str)

            batch_file_name = mpi_directory + "/" +  batch_root
            print("creating batch file " + batch_file_name  + " from " + batch_template)
            f_batch_template = open(batch_template,'r')
            f_batch = open(batch_file_name, 'w')
            for batchster in f_batch_template:
                #print("batchster = " + batchster)
                t1str = batchster;
                t2str = t1str.replace("NUM_NODE", str(i_num_proc))
                t3str = t2str.replace("EXECUTABLE_FILE", "main.exe")
                t4str = t3str.replace("INPUT_FILE", input_root)
                #print("t4str = " + t4str)
                f_batch.write(t4str)
            f_batch.close()
            f_batch_template.close()

            f_input_template = open(input_template,'r')
            input_file_name = mpi_directory + "/" +  input_root
            print("creating input file " + input_file_name  + " from " + input_template)
            f_input = open(input_file_name, 'w')
            for inputster in f_input_template:
                t1str = inputster
                t2str = t1str.replace("MAX_LEVEL", str(i_max_lev))
                f_input.write(t2str)
            f_input.close()
            f_input_template.close()

            batch_command = "\n pushd " +  mpi_directory + "; source " + batch_root + "; popd \n"
            if(args.sbatch_instead_of_source):
                batch_command = "\n pushd " +  mpi_directory + "; sbatch " + batch_root + "; popd \n"
                        
            f_run_all.write( batch_command)
            i_num_proc = 2*i_num_proc
        i_max_lev = i_max_lev + 1
    i_cur_example = i_cur_example + 1

print("Closing run all and changing its mod")
f_run_all.close()
command_str = "chmod +x " + run_all_file_name
print(    command_str)
os.system(command_str)


print("Exiting configure")
exit()
            






