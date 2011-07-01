/*
    Copyright 2007  Ivan Titov and James Henderson 
        
    This file is part of idp.

    idp is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version. See README for other conditions.

    idp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "idp.h"

int read_cand_num(FILE *fp) {
   char buffer[MAX_WGT_LINE];
   
    if (fgets(buffer, MAX_WGT_LINE, fp) == NULL) {
        return 0;
    }
    
    char *s = strtok(buffer, FIELD_SEPS);
    if (s == NULL) {
        return 0;
    }
    int i = atoi(s);
    fgets(buffer, MAX_WGT_LINE, fp);
    
    return i;
}
double read_lprob(FILE *fp) {
   char buffer[MAX_WGT_LINE];
    if (fgets(buffer, MAX_WGT_LINE, fp) == NULL) {
        return 0.;
    }
    
    char *s = strtok(buffer, FIELD_SEPS);
    if (s == NULL) {
        return 0.;
    }
    
    return atof(s);
}

void process_mbr_pred_str(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    char cand_fname[MAX_NAME];
    strcpy(cand_fname, ap->out_file);
    strcat(cand_fname, ".top");

    char out_file[MAX_NAME];
    strcpy(out_file, ap->out_file);
    strcat(out_file, ".mbr_ps");
    
    DEF_FOPEN(ifp, cand_fname, "r");
    DEF_FOPEN(ofp, out_file, "w");

    SENTENCE *sents[ap->cand_num];
    double lprobs[ap->cand_num];
    int ind_num, c = 0;
    printf("[MBR] Reranking...");
    while ((ind_num = read_cand_num(ifp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }
        int i;
        for (i = 0; i < ind_num; i++) {
            lprobs[i] =  read_lprob(ifp);
            sents[i] = read_sentence(mp, ifp, 1);
            ASSERT(sents[i] != NULL);
        }

        double del = lprobs[ind_num - 1];
        for (i = 0; i < ind_num; i++) {
            lprobs[i] -= del;
        }
        
        DEF_ALLOC(res_sent, SENTENCE);
        memcpy(res_sent, sents[0], sizeof(SENTENCE));
       
        int t; 
        for (t = 1; t < res_sent->len + 1; t++) {
            double max_gain = -1; int best_id = -1;
            for(i = 0; i < ind_num; i++) {
                double gain = 0;
                int j;
                for (j = 0; j < ind_num; j++) {
                    int equals = (sents[i]->head[t] == sents[j]->head[t]) 
                        && (strcmp(sents[i]->s_deprel[t], sents[j]->s_deprel[t]) == 0);
                    gain += equals * exp(lprobs[j]);
                }
                if (gain > max_gain) {
                    max_gain = gain;
                    best_id = i;
                }    
            }   
        
            ASSERT(best_id >= 0);
            res_sent->head[t] = sents[best_id]->head[t];
            strcpy(res_sent->s_deprel[t], sents[best_id]->s_deprel[t]);
            res_sent->deprel[t] = sents[best_id]->deprel[t];
            
        }
            
        save_sentence(ofp, res_sent, 1);
        free(res_sent);
        for (i = 0; i < ind_num; i++) {
            free(sents[i]);
            sents[i] = NULL;
        }
        
        if  (c != 0 && c % 1 == 0) {
            printf(".");
            fflush(stdout);
        }
        fflush(stdout);
        c++;
    }
    printf("done. Processed %d sentences\n", c);
    fclose(ifp);
    fclose(ofp);
}


void process_mbr_rerank(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    char cand_fname[MAX_NAME];
    strcpy(cand_fname, ap->out_file);
    strcat(cand_fname, ".top");

    char out_file[MAX_NAME];
    strcpy(out_file, ap->out_file);
    strcat(out_file, ".mbr_rr");
    
    DEF_FOPEN(ifp, cand_fname, "r");
    DEF_FOPEN(ofp, out_file, "w");

    SENTENCE *sents[ap->cand_num];
    double lprobs[ap->cand_num];
    int ind_num, c = 0;
    printf("[MBR] Reranking...");
    while ((ind_num = read_cand_num(ifp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }
        int i;
        for (i = 0; i < ind_num; i++) {
            lprobs[i] =  read_lprob(ifp);
            sents[i] = read_sentence(mp, ifp, 1);
            ASSERT(sents[i] != NULL);
        }

        double del = lprobs[ind_num - 1];
        for (i = 0; i < ind_num; i++) {
            lprobs[i] -= del;
        }

        double max_gain = -1; int best_id = -1;
        for(i = 0; i < ind_num; i++) {
            double gain = 0;
            int j;
            for (j = 0; j < ind_num; j++) {
                gain += get_matched_syntax(sents[i], sents[j], 1) * exp(lprobs[j] * ap->mbr_coeff);
            }
            if (gain > max_gain) {
                max_gain = gain;
                best_id = i;
            }    
        }   
        
        ASSERT(best_id >= 0);
        save_sentence(ofp, sents[best_id], 1);

        for (i = 0; i < ind_num; i++) {
            free(sents[i]);
            sents[i] = NULL;
        }
        if  (c != 0 && c % 1 == 0) {
            printf(".");
            fflush(stdout);
        }
        fflush(stdout);
        c++;
    }
    printf("done. Processed %d sentences\n", c);
    fclose(ifp);
    fclose(ofp);
}


int print_usage() {
    fprintf(stderr, "Usage: ./idp_mbr [-rerank|-pred-tree|-pred-str] settings_file ADDITIONAL_PARAMETER=VALUE\n");
    fprintf(stderr,"\t-rerank\tperforms MBR reranking\n");
    fprintf(stderr,"\t-pred-tree\tpredicts a tree with the smallest risk\n");
    fprintf(stderr,"\t-pred-str\tpredicts a dependency structure with the smallest risk\n");
    fprintf(stderr,"Additional parameters are used to change/add parameters defined in settings_file\n");
    exit(1);
}


int main(int argc, char **argv) {
    printf("Minimum Bayes Risk Reranker for Dependency Parsing\n");
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
            
    printf("MP_SIZE: %d\n", sizeof(MODEL_PARAMS));
    DEF_ALLOC(mp, MODEL_PARAMS);
    read_io_spec(ap->io_spec_file, mp);
    
    if (ap->mbr_coeff <= 0.) {
        fprintf(stderr, "Error: MBR_COEFF should be positive (now: MBR_COEFF = %f)\n", ap->mbr_coeff);
        exit(1);
    }
    
    if (strcmp(mode, "-rerank") == 0) {
        process_mbr_rerank(ap, mp);
    } else if (strcmp(mode, "-pred-tree") == 0) {
        fprintf(stderr, "Error: MBR tree prediction is not yet supported.");
    } else if (strcmp(mode, "-pred-str") == 0) {
        process_mbr_pred_str(ap, mp);
    } else {
        fprintf(stderr, "Error: mode '%s' is not supported\n", mode);
        print_usage();
    }
    
    free(ap);
    free(mp);
    mp = NULL;
    return 0; 
}    

