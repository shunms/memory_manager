#
# START OF The MAKEFILE
#

#
# First, set up the commands to compile and link C programs
#
COMPILE = gcc

#
# The following is the first rule in the Makefile and by default, it will
# be chosen whenever the command make is entered without any arguments.
#
# The general form of a rule is as follows:
#
# target: <space separated list of dependencies for the target>
#  <TAB>   <The action to perform whenever the target is out of date>
#

#
# 
#

# If supplied on the make command line, apply the supplied values of
# Num_Process

ifdef MEM_SIZE
DEF1 = -D"TOTAL_MEM_SIZE=$(MEM_SIZE)"
endif

all:
				$(COMPILE) $(DEF1) -pthread -o mem_mgr common.c mem_mgr.c main.c allocator.c user.c
debug:
				$(COMPILE) $(DEF1) -pthread -g -o mem_mgr common.c mem_mgr.c main.c allocator.c user.c 

clean:
				/bin/rm -f mem_mgr
#
# END OF The MAKEFILE
#

