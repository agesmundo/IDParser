# Makefile for ISBN parser

CC = gcc 
LD = gcc  

#CFLAGS = -pg -g  -Wall
#LDFLAGS = -pg -g -lm -Wall -lgsl -lgslcblas
#CFLAGS =  -Wall 
#LDFLAGS = -lm -Wall -lgsl -lgslcblas
CFLAGS = -O0 -g3  -Wall
LDFLAGS = -O0 -g3 -lm -Wall -lgsl -lgslcblas
SRC = ../src/

all: idp_exe 
#idp_mbr_exe idp_reranker_exe
	
#idp_reranker_exe: all_non_exe
#	$(LD) $(LDFLAGS) idp_reranker.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o  idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o -o ../bin/dd_reranker_debug $(libs)

#idp_mbr_exe: all_non_exe
#	$(LD) $(LDFLAGS) idp_mbr.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o -o ../bin/mbr_reranker_debug $(libs)

idp_exe:  all_non_exe
	$(LD) $(LDFLAGS) idp_main.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o -o ../bin/idp_debug    $(libs)

all_non_exe: idp_main.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o  idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o 

#idp_mbr.o 
#idp_reranker.o

%.o: $(SRC)%.c $(SRC)idp.h $(SRC)idp_io_spec.h
	$(CC) -c $(CFLAGS)  $< -o $@

.PHONY: clean
clean: 
	rm -f *.o *.tcov *.d core *.stackdump 
	rm -f ../idp ../mbr_reranker ../dd_reranker
     
