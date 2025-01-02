#!/usr/bin/python
from argparse import *
import os
import glob
import platform
from datetime import date
import signal
import subprocess
import re

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
        
#./_archive_no_mains_no_hdf/_prch_strong_3_5_2024/old_helmholtz_op.case_3.dim_2.opt_high.debug_false/4_procs/pout.0: RMultiGrid::solveNoInitResid:   Final residual norm  = 3.135270e-13, Multigrid iterations = 13
today = date.today()

print("Today's date:", today)

parser = ArgumentParser()

parser.add_argument('--batch', type=str, help='batch file template'   ,default="../_batch_templates/spencer.batch")
parser.add_argument('--max_num_proc', type=int, help='max number of processors for each run'   ,default='8')
parser.add_argument('--min_num_proc', type=int, help='min number of processors for each run'   ,default='4')
parser.add_argument('--prefix', type=str, help='name of test["mpi_strong"]',default="mpi_strong")
parser.add_argument('--max_lev_min', type=int, help='minimum testing max level (0)',default='0')
parser.add_argument('--max_lev_max', type=int, help='maximum testing max level (9)',default='9')


args = parser.parse_args()
data_directory   =           "_mpi_strong_12_19_2024"
summary_file_name= "summary_of_mpi_strong_12_19_2024.tex"
print(args)
home_str = os.getcwd();
print ("homedir = " + home_str)
today_str =str(today.month) + "_" + str(today.day) + "_" + str(today.year)


#This is just an ordered list of directory/input file/prefix/dim combinations
i_cur_example = 0

driver_dir     = "4586"
i_dim          =  4586
i_found        =  0

if(i_cur_example == 0):
    driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
    i_dim          = 2
    example_prefix = "tests_profile_circle"
    i_found = 1
elif(i_cur_example == 1):
    driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
    i_dim          = 2
    example_prefix = "tests_profile_square"
    i_found = 1
elif(i_cur_example == 2):
    driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
    i_dim          = 3
    example_prefix = "tests_profile_circle"
    i_found = 1
elif(i_cur_example == 3):
    driver_dir     = home_str + "/../../Tests/Electrostatics/Profile"
    i_dim          = 3
    example_prefix = "tests_profile_square"
    i_found = 1

if(i_found == 0):
    print("configure_strong_scaling: logic error")
    exit()

i_opt = 1
i_deb = 1
i_dim = 2
i_mpi = 1

print("driver_dir    [" + str(i_cur_example)  +  "] = " + driver_dir)
print("i_dim         [" + str(i_cur_example)  +  "] = " + str(i_dim))
print("example_prefix[" + str(i_cur_example)  +  "] = " + example_prefix)
funk      =              class_status(example_prefix, driver_dir, i_opt, i_deb, i_dim, i_mpi)


print ("summary_file_name = " + summary_file_name)

top_directory = home_str + "/" + data_directory

print("home_str" + home_str)
print("top_directory=" + top_directory)
print("data_directory=" + data_directory)

label_str = "tab::data_reduction_table_" + today_str
caption_str = "Performance data for " + data_directory
f_summary       = open( summary_file_name,'w')
f_summary.write("\\begin{small} \n ")
f_summary.write("\\begin{table} \n ")
f_summary.write("\\begin{center}\n ")
f_summary.write("\\begin{tabular}{|c|c|c|c|c|c||c|} \\hline \n")
f_summary.write("Max Lev & use eb tags & $N_p$ & bot solve &  Final $|R|$  &  Iter & Main Time \\\\  \n")

print("begin loop through i_max_lev")
i_max_lev = args.max_lev_min
while i_max_lev <= args.max_lev_max:
    
    runs_directory = top_directory + "/_max_lev" + str(i_max_lev) + "_" + funk.all_status + "_runs"
    print("top_directory = " + top_directory)
    print("runs_directory = " + runs_directory)

    print("begin loop through i_use_eb")
    i_use_eb = 0
    while i_use_eb < 2:
        use_eb_str = "USE_EB_4586"
        use_eb_label = "4586"
        if(i_use_eb == 0):
            use_eb_str = "USE_EB_TAGS_FALSE"
            use_eb_label = "false"
        if(i_use_eb == 1):
            use_eb_str = "USE_EB_TAGS_TRUE"
            use_eb_label = "true"
        print("begin loop through num_proc")
        i_num_proc = args.min_num_proc
        while i_num_proc <= args.max_num_proc:
            proc_str = str(i_num_proc) + "_procs"

            print("begin loop through i_bot_solver")
            i_bot_solver = 0
            while(i_bot_solver < 3):
                bot_solver_type  = "solver4586"
                bot_solver_label = "4586"
                if(i_bot_solver == 0):
                    bot_solver_type  = "simple"
                    bot_solver_label = "simp"
                if(i_bot_solver == 1):
                    bot_solver_type = "bicgstab"
                    bot_solver_label = "bicg"
                if(i_bot_solver == 2):
                    bot_solver_type = "gmres"
                    bot_solver_label = "gmrs"

                mpi_directory = runs_directory + "/_" + str(i_num_proc)  + "_procs_" + use_eb_str + "_botsolve_" + bot_solver_type
                
                print("runs_directory  = " + runs_directory)
                print("mpi_directory  = "  + mpi_directory)

                main_str  = "main_time"
                resi_str = "norm_res"
                opt_status = "opt_high"
                pout_name = mpi_directory + "/pout.0"
                time_name = mpi_directory + "/time.table.0"
                comm_str_resi  = "grep Final_residual  " + pout_name
                comm_str_iter  = "grep Final_iteration " + pout_name
                comm_str_time  = "grep Total_Time      " + time_name
                has_pout =  os.path.exists(pout_name) 
                has_time =  os.path.exists(time_name) 
                if( has_pout ):
                    print("YES pout file  found in "  + mpi_directory)
                    #exit()
                else:                           
                    print("NO  pout file  found in "  + mpi_directory)
                    #exit()
                
                if( has_time ):                 
                    print("YES time file  found in "  + mpi_directory)
                else:                                   
                    print("NO  time file  found in "  + mpi_directory)

                if (has_time and has_pout):
                    print("YES both files found in "  + mpi_directory)
                    output_str_resi  = "4586"
                    output_str_iter  = "4586"
                    output_str_time  = "4586"
                    bork_flag = 0
                    try:
                        output_str_resi  = subprocess.check_output(comm_str_resi , shell=True)
                    except subprocess.CalledProcessError as err_resi:
                        bork_flag = 1
                        print(err_resi.output)
                            
                    try:
                        output_str_iter  = subprocess.check_output(comm_str_iter , shell=True)
                    except subprocess.CalledProcessError as err_iter:
                        bork_flag = 1
                        print (err_iter.output)

                    try:
                        output_str_time  = subprocess.check_output(comm_str_time , shell=True)
                    except subprocess.CalledProcessError as err_time:
                        bork_flag = 1
                        print ("borkflag = 1" + err_time.output)

                    if(bork_flag == 0):
                        resi_list =  output_str_resi.split()
                        iter_list =  output_str_iter.split()
                        time_list =  output_str_time.split()
                        resi_len  = len(resi_list)
                        iter_len  = len(iter_list)
                        time_len  = len(time_list)
                        #print( resi_list[resi_len-1] )
                        #print( iter_list[iter_len-1] )
                        #print( time_list[time_len-1] )
                        resi_field = resi_list[resi_len-1]
                        iter_field = iter_list[iter_len-1]
                        time_field = time_list[time_len-1]
                        #print("resi_field   = " + resi_field)
                        #print("iter_field   = " + iter_field)
                        #print("time_field   = " + time_field)
                        #print("comm_str_time= " + comm_str_time)
                        #exit()
                        
                        file_entry = str(i_max_lev) + " & " +  use_eb_label + " & " + str(i_num_proc) + " & " + bot_solver_label + " & "  + resi_field + " & " + iter_field  + " & " + time_field + "\\\\";
                        f_summary.write(file_entry + "\n");
                    else:
                        print_str = "borkflag causes this case to be skipped "
                            
                else:
                    print("skipping this case for lack of data")

                i_bot_solver = i_bot_solver + 1
                print("end loop over i_bot_solver")
            i_num_proc = i_num_proc * 2
            print("end loop over num_procs")

        i_use_eb = i_use_eb + 1
        print("end loop over i_use_eb")

    print("end loop through i_max_lev")
    i_max_lev = i_max_lev + 1

f_summary.write("\\end{tabular} \n")
f_summary.write("\\end{center}   \n")
f_summary.write("\\label{" +     label_str + "} \n") 
f_summary.write("\\caption{" + caption_str + "} \n" )
f_summary.write("\\end{table} \n")
f_summary.write("\\end{small} \n")

print("closing summary and exiting")
f_summary.close()
        
