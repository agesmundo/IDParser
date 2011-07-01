/*
    Copyright 2007  Ivan Titov and James Henderson 
        
    This file is part of ISBN dependency parser (idp).

    ISBN dependency parser (idp) is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version. See README for other conditions.

    ISBN dependency parser (idp) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "idp.h"


int print_usage() {
    fprintf(stderr, "Usage: ./idp [-parse|-train] settings_file [ADDITIONAL_PARAMETER=VALUE]*\n");
    fprintf(stderr,"\t-train  trains the model\n");
    fprintf(stderr,"\t-parse  parses the files\n");
    fprintf(stderr,"Additional parameters are used to change/add parameters defined in settings_file\n");
    exit(1);
}


void process_train(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    validation_training(ap, mp);
}

void process_parse(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    char best_weights_fname[MAX_NAME];
    strcpy(best_weights_fname, ap->model_name);
    strcat(best_weights_fname, ".best.wgt");
    load_weights(ap, mp, best_weights_fname);
    testing_epoch(ap, mp);
}

void print_header() {
	printf("IBN Dependency Parser, version %s\n", _VERSION);
	printf("(c) Ivan Titov and James Henderson, 2007\n\n");
    printf("IBN_PARSER is free software, you can redistribute it and/or modify it under");
    printf("the terms of the GNU General Public License.");
    printf(" See README for usage instructions and licensing information.\n");
}


int main(int argc, char **argv) {
    printf("IBN Parser\n");
    
    if (argc < 3) {
        print_usage();
    }
   
    char *mode = argv[1];
    char *set_fname = argv[2];
    
    APPROX_PARAMS *ap = read_params(set_fname);
    rewrite_parameters(ap, argc, argv, 3);
    
    char *sap = get_params_string(ap);
    printf("Parameters used\n===============\n%s===============\n", sap);
    free(sap);
            
    
    srand48(ap->seed);
  
    DEF_ALLOC(mp, MODEL_PARAMS);
    read_io_spec(ap->io_spec_file, mp);
    read_ih_links_spec(ap->ih_links_spec_file, mp, mp->ih_links_specs, &mp->ih_links_num);
    read_hh_links_spec(ap->hh_links_spec_file, mp);
    allocate_weights(ap, mp);
    
    if (strcmp(mode, "-parse") == 0) {
        process_parse(ap, mp);
    } else if (strcmp(mode, "-train") == 0) {
        if (ap->train_dataset_size == 0) {
            ap->train_dataset_size = MAXINT;
            //fprintf(stderr, "Warning: TRAIN_DATASET_SIZE is not set, using TRAIN_DATASET_SIZE = %d\n", ap->train_dataset_size);
        }
        process_train(ap, mp);
    } else {
        fprintf(stderr, "Error: mode '%s' is not supported\n", mode);
        print_usage();
    }
    
    free(ap);
    free_weights(mp);
    free(mp);
    mp = NULL;
    return 0; 
}    

