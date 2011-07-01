# Makefile for ISBN parser

CC = gcc 
LD = gcc  

#CFLAGS =  -g  -Wall
#LDFLAGS =  -g -lm -Wall -lgsl -lgslcblas
#CFLAGS =  -Wall 
#LDFLAGS = -lm -Wall -lgsl -lgslcblas
CFLAGS = -O3  -Wall
LDFLAGS = -O3 -lm -Wall -lgsl -lgslcblas
#CFLAGS =  -O3  -Wall -I/user/u4/titov/local/gsl-clai/include/
#LDFLAGS =  -O3 -lm -Wall -lgsl -lgslcblas -L/user/u4/titov/local/gsl-clai/lib



all: idp_exe 
#idp_mbr_exe idp_reranker_exe
	
#idp_reranker_exe: all_non_exe
#	$(LD) $(LDFLAGS) idp_reranker.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o  idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o -o ../bin/dd_reranker $(libs)

#idp_mbr_exe: all_non_exe
#	$(LD) $(LDFLAGS) idp_mbr.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o -o ../bin/mbr_reranker $(libs)

idp_exe:  all_non_exe
	$(LD) $(LDFLAGS) idp_main.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o  -o ../bin/idp    $(libs)

all_non_exe: idp_main.o idp_data_manup.o idp_stack.o idp_queue.o idp_part_tree.o idp_part_srl.o  idp_parse.o idp_dyn_struct.o idp_frame.o idp_estim.o idp_search.o 

#idp_mbr.o 
#idp_reranker.o

#idp_reranker.o: idp_reranker.c idp.h idp_io_spec.h
#	$(CC) -c $(CFLAGS)  idp_reranker.c -o idp_reranker.o

#idp_mbr.o: idp_mbr.c idp.h idp_io_spec.h
#	$(CC) -c $(CFLAGS)  idp_mbr.c -o idp_mbr.o
	
idp_search.o: idp_search.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_search.c -o idp_search.o
	
idp_estim.o: idp_estim.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_estim.c -o idp_estim.o

idp_frame.o: idp_frame.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_frame.c -o idp_frame.o

idp_data_manup.o: idp_data_manup.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_data_manup.c -o idp_data_manup.o	

idp_dyn_struct.o: idp_dyn_struct.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_dyn_struct.c -o idp_dyn_struct.o	
	
idp_parse.o: idp_parse.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_parse.c -o idp_parse.o	

idp_stack.o: idp_stack.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS) idp_stack.c -o idp_stack.o

idp_queue.o: idp_queue.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS) idp_queue.c -o idp_queue.o

idp_part_tree.o: idp_part_tree.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS) idp_part_tree.c -o idp_part_tree.o

idp_part_srl.o: idp_part_srl.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS) idp_part_srl.c -o idp_part_srl.o

#idp_common_api.o: idp_common_api.c idp.h idp_io_spec.h
#	$(CC) -c $(CFLAGS)  idp_common_api.c -o idp_common_api.o

idp_main.o: idp_main.c idp.h idp_io_spec.h
	$(CC) -c $(CFLAGS)  idp_main.c -o idp_main.o

#idp_ann.o: idp_ann.c idp.h idp_io_spec.h
#	$(CC) -c $(CFLAGS) idp_ann.c -o idp_ann.o

.PHONY: clean
clean: 
	rm -f *.o *.tcov *.d core *.stackdump 
	rm -f ../idp ../mbr_reranker ../dd_reranker
