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
#include <math.h>

typedef struct expl_comp {
    //update time 
    int ut[ACTION_NUM][MAX_DEPREL_SIZE + 1];
    //feature weight
    double b[ACTION_NUM][MAX_DEPREL_SIZE + 1];
} EXPL_COMP;

typedef struct dd_comp {
    double *b;//[MAX_OUT_SIZE];
    double **w;//[MAX_OUT_SIZE][MAX_HID_SIZE];
    int *ut;//[MAX_OUT_SIZE];
} DD_COMP;


//vector of weights for explicit features
typedef struct expl_vector {
    //input links and input specification
    int comp_num;
    IH_LINK_SPEC ih_links_specs[MAX_IH_LINKS];
    
    //input links and input specification
    EXPL_COMP  
        **ih_link_cpos, //[link_id][cpos]
        **ih_link_deprel, //[link_id][deprel]
        **ih_link_pos, //[link_id][pos]
        **ih_link_lemma, //[link_id][lemma]
        **ih_link_elfeat, //[leank_id][elfeat] - elementary features (decomposed)
        ***ih_link_feat, //[link_id][[pos][feat] - composed features 
        ****ih_link_word, //[link_id][pos][feat][word]
        init_bias, 
        hid_bias, 
        //for decision options
        prev_act_bias[ACTION_NUM],
        prev_deprel_bias[ACTION_NUM][MAX_DEPREL_SIZE];
    
} EXPL_VECTOR;

//vector of weight for data-defined features
typedef struct dd_vector {
     DD_COMP 
        out_link_act, 
        out_link_la_label, 
        out_link_ra_label, 
        out_link_pos,
        *out_link_feat, //[pos]
        **out_link_word; //[pos][feat] 

} DD_VECTOR;


typedef struct dd_score_connector {
    int attached;
    DD_COMP *dd_comps[MAX_BEAM + 1];
} DD_CONNECTOR;



//weighting vector
typedef struct comb_vector {
    double lprob_feat;
    int time;
    
    EXPL_VECTOR expl_vector;
    DD_VECTOR dd_vector;
} COMB_VECTOR;



int attach_dd_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, DD_VECTOR *v) {
     
    
    //out_link_act
    if (mp->out_link_act.info == NULL) {
        ALLOC(mp->out_link_act.info, DD_CONNECTOR);
    }
    DD_CONNECTOR *ddc;
       
    ddc = (DD_CONNECTOR *) mp->out_link_act.info;
    int attach_id = ddc->attached;
    ddc->dd_comps[ddc->attached++] = &v->out_link_act;
    
    //out_link_la_label
    if (mp->out_link_la_label.info == NULL) {
        ALLOC(mp->out_link_la_label.info, DD_CONNECTOR);
    }

    ddc = (DD_CONNECTOR*) mp->out_link_la_label.info;
    ddc->dd_comps[ddc->attached++] = &v->out_link_la_label;

    //out_link_ra_label
    if (mp->out_link_ra_label.info == NULL) {
        ALLOC(mp->out_link_ra_label.info, DD_CONNECTOR);
    }
    ddc = (DD_CONNECTOR*) mp->out_link_ra_label.info;
    ddc->dd_comps[ddc->attached++] = &v->out_link_ra_label;

    //out_link_pos
    ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
    if (mp->out_link_pos.info == NULL) {
        ALLOC(mp->out_link_pos.info, DD_CONNECTOR);
    }
    ddc = (DD_CONNECTOR*) mp->out_link_pos.info;
    ddc->dd_comps[ddc->attached++] = &v->out_link_pos;
   
    //out_link_feat 
    int p;
    int pos_num = ap->input_offset + mp->pos_info_out.num;
    for (p = 0; p < pos_num; p++) {
        if (mp->out_link_feat[p].info == NULL) {
            ALLOC(mp->out_link_feat[p].info, DD_CONNECTOR);
        }
        ddc = (DD_CONNECTOR*) mp->out_link_feat[p].info;
        ddc->dd_comps[ddc->attached++] = & v->out_link_feat[p];
    }
    
    //out_link_word
    int f;
    for (p = 0; p < pos_num; p++) {
        for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
            if (mp->out_link_word[p][f].info == NULL) {
                ALLOC(mp->out_link_word[p][f].info, DD_CONNECTOR);
            }   
            ddc = (DD_CONNECTOR*) mp->out_link_word[p][f].info;
            ddc->dd_comps[ddc->attached++] = & v->out_link_word[p][f];
        }
    }    

    return attach_id;
}

#define get_dd_comp_from_opt(opt, ref) ((DD_CONNECTOR *) (opt)->out_link->info)->dd_comps[(ref)]


//dettaches dd vectors from weight matrixes, frees all the structures
void dettach_dd_vectors(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    //out_link_act
    if (mp->out_link_act.info != NULL) {
        free(mp->out_link_act.info);
        mp->out_link_act.info = NULL;            
    }
    
    //out_link_la_label
    if (mp->out_link_la_label.info != NULL) {
        free(mp->out_link_la_label.info);
        mp->out_link_la_label.info = NULL;
        
    }

    //out_link_ra_label
    if (mp->out_link_ra_label.info != NULL) {
        free(mp->out_link_ra_label.info);
        mp->out_link_ra_label.info = NULL;
    }

    //out_link_pos
    if (mp->out_link_pos.info != NULL) {
        free(mp->out_link_pos.info);
        mp->out_link_pos.info = NULL;
    }
   
    //out_link_feat 
    int p;
    int pos_num = ap->input_offset + mp->pos_info_out.num;
    for (p = 0; p < pos_num; p++) {
        if (mp->out_link_feat[p].info != NULL) {
            free(mp->out_link_feat[p].info);
            mp->out_link_feat[p].info = NULL;
        }
    }
    
    //out_link_word
    int f;
    for (p = 0; p < pos_num; p++) {
        for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
            if (mp->out_link_word[p][f].info != NULL) {
                free(mp->out_link_word[p][f].info);
                mp->out_link_word[p][f].info = NULL;
            }   
        }
    }    
}

int loc_get_prev_deprel(MODEL_PARAMS *mp, OPTION *opt) {
    int deprel;
    if (opt->previous_act == LA || opt->previous_act == RA) {
        OPTION *new_opt = opt;
        while (!new_opt->new_state) {
            new_opt = new_opt->previous_option;
        }
        while(new_opt->previous_option->out_link != &mp->out_link_ra_label && new_opt->previous_option->out_link != &mp->out_link_la_label) {
            new_opt = new_opt->previous_option;
        }
        deprel = new_opt->previous_output;
        ASSERT(deprel >= 0 && deprel < mp->deprel_num);
    } else {
         deprel = 0;
    }
    return deprel;
}
double comp_element_expl_score(APPROX_PARAMS *ap, MODEL_PARAMS *mp,  EXPL_VECTOR *x,  SENTENCE *sent, OPTION *opt, int corr_out) {
//    ASSERT(opt->inp_num == 0);
    PART_TREE *pt = opt->pt;
   
    double score = 0;
    int act, label;
    if (opt->out_link == &mp->out_link_act) {
        act = corr_out;
        label = mp->deprel_num;

    //hacky to be able to use previous version which contains a bug
    } else if ((opt->previous_act == LA || opt->previous_act == RA)  && (opt->out_link == &mp->out_link_la_label || opt->out_link == &mp->out_link_ra_label)) {
        act = opt->previous_act;
        label = corr_out;        
    } else {
        //we do  not predict words with perceptron
        return score;
    }

    //create links using specifications
    //don't forget hid_bias and init_bias
    if (opt->previous_option == NULL) {
        score += x->init_bias.b[act][label];
        return score;
    } else {
        score += x->hid_bias.b[act][label];
        score += x->prev_act_bias[opt->previous_act].b[act][label];
        
        int deprel = loc_get_prev_deprel(mp, opt);
        score += x->prev_deprel_bias[opt->previous_act][deprel].b[act][label];
    }
    

    int l;
    for (l = 0; l < x->comp_num; l++) {
        IH_LINK_SPEC *spec = &x->ih_links_specs[l];
        int w = get_word_by_route(opt, &spec->node, spec->step_type);  
        
        if (spec->info_type == DEPREL_TYPE) {
            int deprel;
            if (w > 0) {
                deprel = pt->nodes[w].deprel;    
                if (deprel == ROOT_DEPREL) {
                    deprel = mp->deprel_num + 1;
                }
            } else {
                deprel = mp->deprel_num;
            } 
            score += x->ih_link_deprel[l][deprel].b[act][label];                    
        } else if (spec->info_type == POS_TYPE) {
            int pos;
            if (w > 0) {
                pos = sent->pos[w];
            } else {
                pos = mp->pos_info_in.num;
            }
            score += x->ih_link_pos[l][pos].b[act][label];
        } else if (spec->info_type == WORD_TYPE) {
            int pos, feat, word;
            if (w > 0) {
                pos = sent->pos[w];
                feat = sent->feat_in[w];
                word = sent->word_in[w];
            } else {
                pos = mp->pos_info_in.num;
                feat = 0; word = 0;
            }
            score += x->ih_link_word[l][pos][feat][word].b[act][label]; 
        } else if (spec->info_type ==  FEAT_TYPE) {
            if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int pos, feat;
                if (w > 0) {
                    pos = sent->pos[w];
                    feat = sent->feat_in[w];
                } else {
                    pos = mp->pos_info_in.num;
                    feat = 0;
                }
                score += x->ih_link_feat[l][pos][feat].b[act][label];
            }
            if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                if (w > 0) {
                    if (sent->elfeat_len[w] + opt->inp_num > MAX_IH_LINKS_OPT) {
                        fprintf(stderr, "Too many features in an option, exceeded MAX_IH_LINKS_OPT = %d\n", MAX_IH_LINKS_OPT);
                        exit(1);
                    }
                    int i;
                    for (i = 0; i < sent->elfeat_len[w]; i++) {
                        int elfeat = sent->elfeat[w][i];
                        score += x->ih_link_elfeat[l][elfeat].b[act][label];    
                    }
                } else {
                    score += x->ih_link_elfeat[l][mp->elfeat_num].b[act][label];
                }
            }
            
        } else if (spec->info_type == CPOS_TYPE) {
            int cpos;
            if (w > 0) {
                cpos = sent->cpos[w];
            } else {
                cpos = mp->cpos_num;
            }
            score += x->ih_link_cpos[l][cpos].b[act][label];
        } else if (spec->info_type == LEMMA_TYPE) {
            int lemma;
            if (w > 0) {
                lemma  = sent->lemma[w];
            }   else {
                lemma = mp->lemma_num;
            }
            score += x->ih_link_lemma[l][lemma].b[act][label];
        } else {
            ASSERT(0);
        } 
        
    }
    return score;
}

double comp_expl_product(APPROX_PARAMS *ap, MODEL_PARAMS *mp, EXPL_VECTOR *x, SENTENCE *sent, OPTION_LIST *der_first, OPTION_LIST *der_last) {
    OPTION_LIST  *curr_node = der_last->prev->prev;
    int corr_out = der_last->prev->elem->previous_output;
    ASSERT(corr_out >= 0);
    double score = 0.;
    
    while (curr_node != NULL) {
        OPTION *opt = curr_node->elem;
        score += comp_element_expl_score(ap, mp, x, sent, opt,  corr_out);
        corr_out = opt->previous_output;
        curr_node = curr_node->prev;
    }
    return score;
}

double comp_dd_vector_and_product(APPROX_PARAMS *ap, MODEL_PARAMS *mp, DD_VECTOR *curr_x, int curr_x_ref, 
        SENTENCE *sent,  OPTION_LIST * der_first, OPTION_LIST *der_last, DD_VECTOR *dd_vector, int dd_vector_ref) {
    
    OPTION_LIST  *curr_node = der_last->prev->prev;
    int corr_out = der_last->prev->elem->previous_output;
    ASSERT(corr_out >= 0);
    double score = 0;

    while (curr_node != NULL) {
        OPTION *opt = curr_node->elem;
        DD_COMP *curr_dd_comp = get_dd_comp_from_opt(opt, curr_x_ref);
        DD_COMP *feat_dd_comp = get_dd_comp_from_opt(opt, dd_vector_ref); 
         
        feat_dd_comp->b[corr_out] += 1;
        score += curr_dd_comp->b[corr_out]; 
        int h;
        for (h = 0; h < ap->hid_size; h++) {
            feat_dd_comp->w[corr_out][h] += opt->int_layer.mju[h];            
            score += curr_dd_comp->w[corr_out][h] * opt->int_layer.mju[h];  
        }
        
        corr_out = opt->previous_output;
        curr_node = curr_node->prev;
    }
    
    return score;
}

double comp_dd_product(APPROX_PARAMS *ap, MODEL_PARAMS *mp, DD_VECTOR *curr_x, int curr_x_ref,
                SENTENCE *sent,  OPTION_LIST * der_first, OPTION_LIST *der_last) {

    OPTION_LIST  *curr_node = der_last->prev->prev;
    int corr_out = der_last->prev->elem->previous_output;
    ASSERT(corr_out >= 0);
    double score = 0;

    while (curr_node != NULL) {
        OPTION *opt = curr_node->elem;
        DD_COMP *curr_dd_comp = get_dd_comp_from_opt(opt, curr_x_ref);
         
        score += curr_dd_comp->b[corr_out]; 
        int h;
        for (h = 0; h < ap->hid_size; h++) {
            score += curr_dd_comp->w[corr_out][h] * opt->int_layer.mju[h];  
        }
        
        corr_out = opt->previous_output;
        curr_node = curr_node->prev;
    }
    
    return score;
    

}
double comp_score(APPROX_PARAMS *ap, MODEL_PARAMS *mp, COMB_VECTOR *curr_x, int curr_x_ref, SENTENCE *sent, double lprob_feat, 
        OPTION_LIST * der_first, OPTION_LIST *der_last) {
            
    double score = 0;        
    if (ap->use_dd_comp) {
        comp_sentence(ap, mp, der_first);
        score += ap->dd_w * comp_dd_product(ap, mp, &(curr_x->dd_vector), curr_x_ref, sent, der_first, der_last);    
    }
    if (ap->use_expl_comp) {
        score += comp_expl_product(ap, mp, &(curr_x->expl_vector), sent, der_first, der_last);
    }

    return score;
}

double comp_score_and_dd_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, COMB_VECTOR *curr_x, int curr_x_ref, SENTENCE *sent, double lprob_feat, 
        OPTION_LIST * der_first, OPTION_LIST *der_last, DD_VECTOR *dd_vector, int dd_vector_ref) {
    double score = 0;        
            
    if (ap->use_dd_comp) {
        comp_sentence(ap, mp, der_first);
        score += ap->dd_w * comp_dd_vector_and_product(ap, mp, &(curr_x->dd_vector), curr_x_ref, sent, der_first, der_last, dd_vector, dd_vector_ref);    
    }
    if (ap->use_expl_comp) {
        score += comp_expl_product(ap, mp, &(curr_x->expl_vector), sent, der_first, der_last);
    }

    return score;
}

void element_update_expl(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, EXPL_VECTOR *x, EXPL_VECTOR *sx, SENTENCE *sent, OPTION *opt, int corr_out, double k) {
    PART_TREE *pt = opt->pt;
   
    int act, label;
    if (opt->out_link == &mp->out_link_act) {
        act = corr_out;
        label = mp->deprel_num;

    //hacky to be able to use previous version which contains a bug
    // TODO: correct
    } else if ((opt->previous_act == LA || opt->previous_act == RA)  && (opt->out_link == &mp->out_link_la_label || opt->out_link == &mp->out_link_ra_label)) {
        act = opt->previous_act;
        label = corr_out;        
    } else {
        //we do  not predict words with perceptron
        return;
    }

#define UPDATE_X(bias)\
    {\
        sx->bias.b[act][label] += 0.01 * (curr_time - x->bias.ut[act][label]) * x->bias.b[act][label];\
        x->bias.b[act][label] += k;\
        x->bias.ut[act][label] = curr_time;\
    }

    
    //create links using specifications
    //don't forget hid_bias and init_bias
    if (opt->previous_option == NULL) {
        UPDATE_X(init_bias);
        return;
    } else {
        UPDATE_X(hid_bias);
        UPDATE_X(prev_act_bias[opt->previous_act]);
        
        int deprel = loc_get_prev_deprel(mp, opt);
        UPDATE_X(prev_deprel_bias[opt->previous_act][deprel]);
    }
    

    int l;
    for (l = 0; l < x->comp_num; l++) {
        IH_LINK_SPEC *spec = &x->ih_links_specs[l];
        int w = get_word_by_route(opt, &spec->node, spec->step_type);  
        
        if (spec->info_type == DEPREL_TYPE) {
            int deprel;
            if (w > 0) {
                deprel = pt->nodes[w].deprel;    
                if (deprel == ROOT_DEPREL) {
                    deprel = mp->deprel_num + 1;
                }
            } else {
                deprel = mp->deprel_num;
            }
            UPDATE_X(ih_link_deprel[l][deprel]); 
        } else if (spec->info_type == POS_TYPE) {
            int pos;
            if (w > 0) {
                pos = sent->pos[w];
            } else {
                pos = mp->pos_info_in.num;
            }
            UPDATE_X(ih_link_pos[l][pos]);
        } else if (spec->info_type == WORD_TYPE) {
            int pos, feat, word;
            if (w > 0) {
                pos = sent->pos[w];
                feat = sent->feat_in[w];
                word = sent->word_in[w];
            } else {
                pos = mp->pos_info_in.num;
                feat = 0; word = 0;
            }
            UPDATE_X(ih_link_word[l][pos][feat][word]);
        } else if (spec->info_type ==  FEAT_TYPE) {
            if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int pos, feat;
                if (w > 0) {
                    pos = sent->pos[w];
                    feat = sent->feat_in[w];
                } else {
                    pos = mp->pos_info_in.num;
                    feat = 0;
                }
                UPDATE_X(ih_link_feat[l][pos][feat]);
            }
            if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                if (w > 0) {
                    if (sent->elfeat_len[w] + opt->inp_num > MAX_IH_LINKS_OPT) {
                        fprintf(stderr, "Too many features in an option, exceeded MAX_IH_LINKS_OPT = %d\n", MAX_IH_LINKS_OPT);
                        exit(1);
                    }
                    int i;
                    for (i = 0; i < sent->elfeat_len[w]; i++) {
                        int elfeat = sent->elfeat[w][i];
                        UPDATE_X(ih_link_elfeat[l][elfeat]);
                    }
                } else {
                    UPDATE_X(ih_link_elfeat[l][mp->elfeat_num]);
                }
            }
            
        } else if (spec->info_type == CPOS_TYPE) {
            int cpos;
            if (w > 0) {
                cpos = sent->cpos[w];
            } else {
                cpos = mp->cpos_num;
            }
            UPDATE_X(ih_link_cpos[l][cpos]);
        } else if (spec->info_type == LEMMA_TYPE) {
            int lemma;
            if (w > 0) {
                lemma  = sent->lemma[w];
            }   else {
                lemma = mp->lemma_num;
            }
            UPDATE_X(ih_link_lemma[l][lemma]);
        } else {
            ASSERT(0);
        } 
        
    }
}

void update_expl_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, EXPL_VECTOR *curr_x, EXPL_VECTOR *sum_x, SENTENCE *sent, OPTION_LIST *der_last, double coeff) {
        OPTION_LIST  *curr_node = der_last->prev->prev;
        int corr_out = der_last->prev->elem->previous_output;
        ASSERT(corr_out >= 0);

        while (curr_node != NULL) {
            OPTION *opt = curr_node->elem;
            
            element_update_expl(ap, mp, curr_time, curr_x, sum_x, sent, opt,  corr_out, coeff);
                
            corr_out = opt->previous_output;
            curr_node = curr_node->prev;
        }
}

    
void update_dd_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, 
        DD_VECTOR *curr_x, int curr_x_ref,  DD_VECTOR *sum_x, int sum_x_ref, 
        SENTENCE *sent, OPTION_LIST *der_last, DD_VECTOR *feat_v, int feat_v_ref,   double coeff) {
        
    
        OPTION_LIST  *curr_node = der_last->prev->prev;
        int corr_out = der_last->prev->elem->previous_output;
        ASSERT(corr_out >= 0);

        while (curr_node != NULL) {
            OPTION *opt = curr_node->elem;

            DD_COMP *curr_dd_comp = get_dd_comp_from_opt(opt, curr_x_ref);
            DD_COMP *sum_dd_comp = get_dd_comp_from_opt(opt, sum_x_ref); 
            DD_COMP *feat_dd_comp = get_dd_comp_from_opt(opt, feat_v_ref);

            //should be done only once per feat component (the same output can appear several times in the sentence
            //and we have already summed them in feat_dd_comp
            int h;
            sum_dd_comp->b[corr_out] += 0.01 * (curr_time - curr_dd_comp->ut[corr_out]) * curr_dd_comp->b[corr_out];
            curr_dd_comp->b[corr_out] += feat_dd_comp->b[corr_out] * coeff;
            feat_dd_comp->b[corr_out] = 0;

            for (h = 0; h < ap->hid_size; h++) {
                sum_dd_comp->w[corr_out][h] += 0.01 * (curr_time - curr_dd_comp->ut[corr_out]) * curr_dd_comp->w[corr_out][h];
                curr_dd_comp->w[corr_out][h] += feat_dd_comp->w[corr_out][h] * coeff;
                feat_dd_comp->w[corr_out][h] = 0;
            }
            curr_dd_comp->ut[corr_out] = curr_time;
    
            corr_out = opt->previous_output;
            curr_node = curr_node->prev;
        }
}
void rearrange_scores(APPROX_PARAMS *ap, int ind_num, double *lprobs, double *scores) {
    if (ap->dd_kernel_type == DD_TOP_KERNEL) {
        double prob_sum = 0.;
        double weight_sum = 0.;
        int i;
        for (i = 0; i < ind_num; i++) {
            double prob = exp(lprobs[i]);
            prob_sum += prob;
            weight_sum += prob * scores[i];
        }
        
        for (i = 0; i < ind_num; i++) {
            scores[i] = (scores[i] * prob_sum - weight_sum) / (prob_sum - exp(lprobs[i]));
        }
        
        
    } else if (ap->dd_kernel_type == DD_FISHER_KERNEL) {
        //do noothing
    } else {
        ASSERT(0);
    }

}

void comp_lprob_feats(APPROX_PARAMS *ap, int ind_num, double *lprobs, double *lprob_feats) {
    double prob_sum = 0.;
    int i;
    
    if (ap->dd_kernel_type == DD_TOP_KERNEL) {
        for (i = 0; i < ind_num; i++) {
            prob_sum += exp(lprobs[i]);
        }
        for (i = 0; i < ind_num; i++) {
            lprob_feats[i] = lprobs[i] - log(prob_sum - lprobs[i]);
        }   
    } else if (ap->dd_kernel_type == DD_FISHER_KERNEL) {
        for (i = 0; i < ind_num; i++) {
            lprob_feats[i] = lprobs[i];
        }

    } else {
        ASSERT(0);
    }
    
}


int comp_update_coeff(APPROX_PARAMS *ap, int ind_num, int best_id,  double *lprobs, int *matched, double *scores, double *upd_coeffs, double *upd_coeffs_lprob) {
    int i, j;
    double norm = 0;

    for (i = 0; i < ind_num; i++) {
        if (scores[i] >= scores[best_id]) {
            norm += matched[best_id] - matched[i];
            ASSERT(norm >= 0);
        }
    }
    if (norm == 0) {
        //we have chosen a candidate as good as the best
        return 0;
    } 

    for (i = 0; i < ind_num; i++) {
        upd_coeffs_lprob[i] = 0;
        upd_coeffs[i] = 0;
    }

    upd_coeffs_lprob[best_id] = 1.;
    
    for (i = 0; i < ind_num; i++) {
        if (scores[i] >= scores[best_id]) {
            upd_coeffs_lprob[i] += - (matched[best_id] - matched[i]) / norm;
        }
    }
    if (ap->dd_kernel_type == DD_FISHER_KERNEL) {
        for (i = 0; i < ind_num; i++) {
            upd_coeffs[i] = upd_coeffs_lprob[i];
        } 
        
    } else {
        double prob_sum = 0;
        double probs[MAX_BEAM];
        for (i = 0; i < ind_num; i++) {
            probs[i] = exp(lprobs[i]);
            prob_sum += probs[i];
        }
        
        for (j = 0; j < ind_num; j++) {
            for (i = 0; i < ind_num; i++) {
                upd_coeffs[j] += upd_coeffs_lprob[i] * ( -probs[j] / (prob_sum - probs[i]));
            }
            upd_coeffs[j] += upd_coeffs_lprob[j] * prob_sum / (prob_sum - probs[j]);
        }    
    }

    return 1;
}


void  update_vectors(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, int ind_num, int best_id,
        COMB_VECTOR *curr_x, int curr_x_ref, COMB_VECTOR *sum_x, int sum_x_ref,
        SENTENCE **sents, int *matched, double *lprobs, double *lprob_feats, double *scores,
        DD_VECTOR **dd_vectors, int *dd_vectors_refs) {

    
    ASSERT(curr_x->time == 0 || (curr_x->time < curr_time));

    OPTION_LIST *der_last, *der_first;  
   

    double upd_coeffs[MAX_BEAM], upd_coeffs_lprob[MAX_BEAM];
    if (!comp_update_coeff(ap, ind_num, best_id, lprobs, matched,  scores, upd_coeffs, upd_coeffs_lprob)) {
        return;
    }
    
    
    sum_x->lprob_feat += (curr_time - curr_x->time) * curr_x->lprob_feat * 0.01;
    
    int i;
    for (i = 0; i < ind_num; i++) {
        if (upd_coeffs[i] != 0 || upd_coeffs_lprob[i] != 0) {
            get_derivation_list(ap, mp, sents[i], &der_first, &der_last, 0);            
            if (ap->use_expl_comp) {
                update_expl_vector(ap, mp, curr_time, &(curr_x->expl_vector), &(sum_x->expl_vector), sents[i], der_last, upd_coeffs[i]);
            }
            if (ap->use_dd_comp) {
                update_dd_vector(ap, mp, curr_time, &(curr_x->dd_vector), curr_x_ref,   &(sum_x->dd_vector), sum_x_ref, sents[i], der_last, 
                        dd_vectors[i], dd_vectors_refs[i], upd_coeffs[i] * ap->dd_w);
            }
            free_derivation(der_last);
            der_last = NULL; der_first = NULL;
            
            curr_x->lprob_feat += lprob_feats[i] * upd_coeffs_lprob[i] * ap->lprob_w ;
        }
    }

    curr_x->time = curr_time;    
}

//final update for sum vector  
void final_update_expl_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, EXPL_VECTOR *v, EXPL_VECTOR *sv) {

    
#define COPY_EXPL_COMP(bias)\
    {\
        int act, rel;\
        for (act = 0; act < ACTION_NUM; act++) { \
            for (rel = 0 ; rel < mp->deprel_num + 1; rel++) {\
                sv->bias.b[act][rel] += 0.01 * (curr_time - v->bias.ut[act][rel]) * v->bias.b[act][rel];\
                v->bias.ut[act][rel] = 0.;\
            }\
        }\
    } 
    
    //save input links
    int l;
    for (l = 0; l < v->comp_num; l++) {
        if (v->ih_links_specs[l].info_type == CPOS_TYPE) {
            int i;
            //+ 1  to represent the fact that there is
            //no word complying to the specification l
            for (i = 0; i < mp->cpos_num + 1; i++) {
                COPY_EXPL_COMP(ih_link_cpos[l][i]);
            }
        } else if (v->ih_links_specs[l].info_type == DEPREL_TYPE) {
            int i;
            // +2 : for ROOT relationship and to represent fact that there is
            // not word which complies with the specification l
            for (i = 0; i < mp->deprel_num + 2; i++) {
                   COPY_EXPL_COMP(ih_link_deprel[l][i]);
            }   
        } else if (v->ih_links_specs[l].info_type == POS_TYPE) {
            int i;
            for (i = 0; i < mp->pos_info_in.num + 1; i++) {
                COPY_EXPL_COMP(ih_link_pos[l][i]);
            }   
        } else if (v->ih_links_specs[l].info_type == LEMMA_TYPE) {
            int i;
            for (i = 0; i < mp->lemma_num + 1; i++) {
                COPY_EXPL_COMP(ih_link_lemma[l][i]);
            }   
        } else if (v->ih_links_specs[l].info_type == FEAT_TYPE) {
            if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int p, f;
                for (p = 0; p < mp->pos_info_in.num; p++) {
                    for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                        COPY_EXPL_COMP(ih_link_feat[l][p][f]);
                    }
                }   
                //link for no word
                COPY_EXPL_COMP(ih_link_feat[l][mp->pos_info_in.num][0]);
            } 
            if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int i;
                for (i = 0; i < mp->elfeat_num + 1; i++) {
                    COPY_EXPL_COMP(ih_link_elfeat[l][i]);    
                }
            }
        } else if (v->ih_links_specs[l].info_type ==  WORD_TYPE) {
            int p, f, w;
            for (p = 0; p < mp->pos_info_in.num; p++) {
                for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                    for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
                        COPY_EXPL_COMP(ih_link_word[l][p][f][w]);
                    }
                }
            }   
            //link for no word
            COPY_EXPL_COMP(ih_link_word[l][mp->pos_info_in.num][0][0]);
        } else {
            ASSERT(0);
        }
    }

    COPY_EXPL_COMP(init_bias);
    COPY_EXPL_COMP(hid_bias);

    int a;
    for (a = 0; a < ACTION_NUM; a++) {
        COPY_EXPL_COMP(prev_act_bias[a]);
    }

    for (a = 0; a < ACTION_NUM; a++) {
        int d;
        for (d = 0; d < mp->deprel_num; d++) {
            COPY_EXPL_COMP(prev_deprel_bias[a][d]);
        }
    }        
    
}
void final_update_dd_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, DD_VECTOR *v, DD_VECTOR *sv) {
    //save output weights
#define COPY_DD_COMP(out_link)\
    {\
        sv->out_link.b[t] += 0.01 * (curr_time - v->out_link.ut[t]) * v->out_link.b[t];\
        for (i = 0; i < ap->hid_size; i++) { \
            sv->out_link.w[t][i] +=  0.01 * (curr_time - v->out_link.ut[t]) * v->out_link.w[t][i];\
        }\
        v->out_link.ut[t] = 0;\
    } 
    
    int t, i;
    for (t = 0; t < ACTION_NUM; t++) {
        COPY_DD_COMP(out_link_act);
    }
    
    for (t = 0; t < mp->deprel_num; t++) {
        COPY_DD_COMP(out_link_la_label);
        COPY_DD_COMP(out_link_ra_label);
    }

    ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
    int pos_num = ap->input_offset + mp->pos_info_out.num;
    for (t = 0; t < pos_num; t++) {
        COPY_DD_COMP(out_link_pos);
    } 

    int p;
    for (p = 0; p < pos_num; p++) {
        for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
            COPY_DD_COMP(out_link_feat[p]);
        }
    }
    
    int f;
    for (p = 0; p < pos_num; p++) {
        for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
            for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
                COPY_DD_COMP(out_link_word[p][f]);
            }
        }
    }    
    
}
void final_update_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int curr_time, COMB_VECTOR *v, COMB_VECTOR *sv) {
    if (ap->use_expl_comp) {
        final_update_expl_vector(ap, mp, curr_time, &(v->expl_vector), &(sv->expl_vector));
    }
    if (ap->use_dd_comp) {
        final_update_dd_vector(ap, mp, curr_time, &(v->dd_vector), &(sv->dd_vector));
    }


}

//saves parameters 
void save_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname, COMB_VECTOR *v) {

    FILE *fp;
    if ((fp = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "Error: can't open file %s\n", fname);
        exit(1);
    }

    fprintf(fp, "#lprob\n");
    fprintf(fp, "%.20f\n", v->lprob_feat);
   
   if (ap->use_expl_comp) { 
#define SAVE_EXPL_COMP(ih)\
        {\
            fprintf(fp, "%.20f", (ih).b[0][0]); \
            int act, rel;\
            for (act = 0; act < ACTION_NUM; act++) { \
                for (rel = act == 0 ? 1 : 0; rel < mp->deprel_num + 1; rel++) {\
                    fprintf(fp, " %.20f", (ih).b[act][rel]);\
                }\
            }\
            fprintf(fp, "\n");\
        } 
        
        //save input links
        int l;
        fprintf(fp, "#ih_links\n#======================\n");
        for (l = 0; l < v->expl_vector.comp_num; l++) {
            if (v->expl_vector.ih_links_specs[l].info_type == CPOS_TYPE) {
                int i;
                //+ 1  to represent the fact that there is
                //no word complying to the specification l
                fprintf(fp, "#Link %d, CPOS_TYPE\n", l);
                for (i = 0; i < mp->cpos_num + 1; i++) {
                    SAVE_EXPL_COMP(v->expl_vector.ih_link_cpos[l][i]);
                }
            } else if (v->expl_vector.ih_links_specs[l].info_type == DEPREL_TYPE) {
                int i;
                fprintf(fp, "#Link %d, DEPREL_TYPE\n", l);
                // +2 : for ROOT relationship and to represent fact that there is
                // not word which complies with the specification l
                for (i = 0; i < mp->deprel_num + 2; i++) {
                       SAVE_EXPL_COMP(v->expl_vector.ih_link_deprel[l][i]);
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == POS_TYPE) {
                int i;
                fprintf(fp, "#Link %d, POS_TYPE\n", l);
                for (i = 0; i < mp->pos_info_in.num + 1; i++) {
                    SAVE_EXPL_COMP(v->expl_vector.ih_link_pos[l][i]);
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == LEMMA_TYPE) {
                int i;
                fprintf(fp, "#Link %d, LEMMA_TYPE\n", l);
                for (i = 0; i < mp->lemma_num + 1; i++) {
                    SAVE_EXPL_COMP(v->expl_vector.ih_link_lemma[l][i]);
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == FEAT_TYPE) {
                if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                    int p, f;
                    fprintf(fp, "#Link %d, FEAT_TYPE (COMPOSED)\n", l);
                    for (p = 0; p < mp->pos_info_in.num; p++) {
                        for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                            SAVE_EXPL_COMP(v->expl_vector.ih_link_feat[l][p][f]);
                        }
                    }   
                    //link for no word
                    SAVE_EXPL_COMP(v->expl_vector.ih_link_feat[l][mp->pos_info_in.num][0]);
                } 
                if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                    int i;
                    fprintf(fp, "#Link %d, FEAT_TYPE (ELEMENTARY)\n", l);
                    for (i = 0; i < mp->elfeat_num + 1; i++) {
                        SAVE_EXPL_COMP(v->expl_vector.ih_link_elfeat[l][i]);    
                    }
                }
            } else if (v->expl_vector.ih_links_specs[l].info_type ==  WORD_TYPE) {
                fprintf(fp, "#Link %d, WORD_TYPE\n", l);
                int p, f, w;
                for (p = 0; p < mp->pos_info_in.num; p++) {
                    for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                        for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
                            SAVE_EXPL_COMP(v->expl_vector.ih_link_word[l][p][f][w]);
                        }
                    }
                }   
                //link for no word
                SAVE_EXPL_COMP(v->expl_vector.ih_link_word[l][mp->pos_info_in.num][0][0]);
            } else {
                ASSERT(0);
            }
        }

        fprintf(fp, "#======================\n#init_bias\n");
        SAVE_EXPL_COMP(v->expl_vector.init_bias);
        fprintf(fp, "#======================\n#hid_bias\n");
        SAVE_EXPL_COMP(v->expl_vector.hid_bias);

        int a;
        fprintf(fp, "#======================\n#prev_act_bias\n");
        for (a = 0; a < ACTION_NUM; a++) {
            SAVE_EXPL_COMP(v->expl_vector.prev_act_bias[a]);
        }

        fprintf(fp, "#======================\n#prev_deprel_bias\n");
        for (a = 0; a < ACTION_NUM; a++) {
            int d;
            for (d = 0; d < mp->deprel_num; d++) {
                SAVE_EXPL_COMP(v->expl_vector.prev_deprel_bias[a][d]);
            }
        }        
        
    }
    if (ap->use_dd_comp) {
        
#define SAVE_DD_COMP(out_link)\
        {\
            fprintf(fp, "%.20f", (out_link).b[t]);\
            for (i = 0; i < ap->hid_size; i++) { \
                fprintf(fp, " %.20f", (out_link).w[t][i]);\
            }\
            fprintf(fp, "\n");\
        } 
        fprintf(fp, "#======================\n#out_links\n#======================\n"); 
        
        int t, i;
        fprintf(fp,"#out_link_act\n");
        for (t = 0; t < ACTION_NUM; t++) {
            SAVE_DD_COMP(v->dd_vector.out_link_act);
        }
        
        fprintf(fp,"#out_link_la_label/ra_label\n");
        for (t = 0; t < mp->deprel_num; t++) {
            SAVE_DD_COMP(v->dd_vector.out_link_la_label);
            SAVE_DD_COMP(v->dd_vector.out_link_ra_label);
        }

        ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
        int pos_num = ap->input_offset + mp->pos_info_out.num;
        fprintf(fp,"#out_link_pos\n");
        for (t = 0; t < pos_num; t++) {
            SAVE_DD_COMP(v->dd_vector.out_link_pos);
        } 

        fprintf(fp,"#out_link_feat\n");
        int p;
        for (p = 0; p < pos_num; p++) {
            for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
                SAVE_DD_COMP(v->dd_vector.out_link_feat[p]);
            }
        }
        
        fprintf(fp,"#out_link_word\n");
        int f;
        for (p = 0; p < pos_num; p++) {
            for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
                for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
                    SAVE_DD_COMP(v->dd_vector.out_link_word[p][f]);
                }
            }
        }    
    }
    
    fclose(fp);
}

//loads parameters 
void load_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname, COMB_VECTOR *v) {

    FILE *fp;
    if ((fp = fopen(fname, "r")) == NULL) {
        fprintf(stderr, "Error: can't open file %s\n", fname);
        exit(1);
    }

    char buffer[MAX_WGT_LINE];
    do { 
        err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); 
    } while (buffer[0] == '#'); 

    v->lprob_feat = atof(buffer);

    if (ap->use_expl_comp) {
#define READ_EXPL_COMP(ih, name)\
        {\
            do { \
                err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); \
            } while (buffer[0] == '#'); \
            (ih).b[0][0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), name));\
            int act, rel;\
            for (act = 0; act < ACTION_NUM; act++) {\
                for (rel =  act == 0 ? 1 : 0; rel < mp->deprel_num + 1; rel++) {\
                    (ih).b[act][rel] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name)); \
                }\
            }\
        } 
        
        //read input links
        int l;
            
        for (l = 0; l < v->expl_vector.comp_num; l++) {
            if (v->expl_vector.ih_links_specs[l].info_type == CPOS_TYPE) {
                int i;
                //+ 1  to represent the fact that there is
                //no word complying to the specification l
                for (i = 0; i < mp->cpos_num + 1; i++) {
                    READ_EXPL_COMP(v->expl_vector.ih_link_cpos[l][i], "ih_link_cpos");
                }
            } else if (v->expl_vector.ih_links_specs[l].info_type == DEPREL_TYPE) {
                int i;
                // +2 : for ROOT relationship and to represent fact that there is
                // not word which complies with the specification l
                for (i = 0; i < mp->deprel_num + 2; i++) {
                       READ_EXPL_COMP(v->expl_vector.ih_link_deprel[l][i],"ih_link_deprel");
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == POS_TYPE) {
                int i;
                for (i = 0; i < mp->pos_info_in.num + 1; i++) {
            //        READ_EXPL_COMP(v->expl_vector.ih_link_pos[l][i], "ih_link_pos");
                    do { 
                        err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); 
                    } while (buffer[0] == '#'); 
                    (v->expl_vector.ih_link_pos[l][i]).b[0][0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), ""));
                    int act, rel;
                    for (act = 0; act < ACTION_NUM; act++) {
                        for (rel =  act == 0 ? 1 : 0; rel < mp->deprel_num + 1; rel++) {
                            v->expl_vector.ih_link_pos[l][i].b[act][rel] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), "")); 
                        }
                    }
            
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == LEMMA_TYPE) {
                int i;
                for (i = 0; i < mp->lemma_num + 1; i++) {
                    READ_EXPL_COMP(v->expl_vector.ih_link_lemma[l][i], "ih_link_lemma");
                }   
            } else if (v->expl_vector.ih_links_specs[l].info_type == FEAT_TYPE) {
                if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                    int p, f;
                    for (p = 0; p < mp->pos_info_in.num; p++) {
                        for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                            READ_EXPL_COMP(v->expl_vector.ih_link_feat[l][p][f], "ih_link_feat");
                        }
                    }   
                    //link for no word
                    READ_EXPL_COMP(v->expl_vector.ih_link_feat[l][mp->pos_info_in.num][0], "ih_link_feat");
                }
                if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                    int i;
                    for (i = 0; i < mp->elfeat_num + 1; i++) {
                        READ_EXPL_COMP(v->expl_vector.ih_link_elfeat[l][i], "ih_link_elfeat");
                    }
                }
            } else if (v->expl_vector.ih_links_specs[l].info_type ==  WORD_TYPE) {
                int p, f, w;
                for (p = 0; p < mp->pos_info_in.num; p++) {
                    for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                        for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
                            READ_EXPL_COMP(v->expl_vector.ih_link_word[l][p][f][w], "ih_link_word");
                        }
                    }
                }   
                //link for no word
                READ_EXPL_COMP(v->expl_vector.ih_link_word[l][mp->pos_info_in.num][0][0], "ih_link_word");
            } else {
                ASSERT(0);
            }
        }

        READ_EXPL_COMP(v->expl_vector.init_bias, "init_bias");
        READ_EXPL_COMP(v->expl_vector.hid_bias, "hid_bias");

        int a;
        for (a = 0; a < ACTION_NUM; a++) {
            READ_EXPL_COMP(v->expl_vector.prev_act_bias[a], "prev_act_bias");
        }

        for (a = 0; a < ACTION_NUM; a++) {
            int d;
            for (d = 0; d < mp->deprel_num; d++) {
                READ_EXPL_COMP(v->expl_vector.prev_deprel_bias[a][d], "prev_deprel_bias");
            }
        }        
    } 
        
    if (ap->use_dd_comp) {
#define READ_DD_COMP(out_link, name)\
        {\
            do { \
                err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); \
            } while (buffer[0] == '#'); \
            (out_link).b[t] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), name));\
            for (i = 0; i < ap->hid_size; i++) { \
                (out_link).w[t][i] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name)); \
            }\
        } 
         
        
        int t, i;
        for (t = 0; t < ACTION_NUM; t++) {
            READ_DD_COMP(v->dd_vector.out_link_act, "out_link_act");
        }
        
        for (t = 0; t < mp->deprel_num; t++) {
            READ_DD_COMP(v->dd_vector.out_link_la_label, "out_link_la_label");
            READ_DD_COMP(v->dd_vector.out_link_ra_label, "out_link_ra_label");
        }

        ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
        int pos_num = ap->input_offset + mp->pos_info_out.num;
        for (t = 0; t < pos_num; t++) {
            READ_DD_COMP(v->dd_vector.out_link_pos, "out_link_pos");
        } 
        int p;
        for (p = 0; p < pos_num; p++) {
            for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
                READ_DD_COMP(v->dd_vector.out_link_feat[p], "out_link_feat");
            }
        }
        
        int f;
        for (p = 0; p < pos_num; p++) {
            for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
                for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
                    READ_DD_COMP(v->dd_vector.out_link_word[p][f], "out_link_word");
                }
            }
        }    
    }
        
    fclose(fp);

}

//allocates memory space for all the weights
//should be invoked after reading hh, ih and io specs
void allocate_expl_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, EXPL_VECTOR *v) {

    //allocate input links
    int l;
    
    ALLOC_ARRAY(v->ih_link_cpos, EXPL_COMP *, v->comp_num);
    ALLOC_ARRAY(v->ih_link_deprel, EXPL_COMP *, v->comp_num);
    ALLOC_ARRAY(v->ih_link_pos, EXPL_COMP *, v->comp_num);
    ALLOC_ARRAY(v->ih_link_lemma, EXPL_COMP *, v->comp_num);
    if (ap->inp_feat_mode == FEAT_MODE_COMPOSED) {
        ALLOC_ARRAY(v->ih_link_feat, EXPL_COMP **, v->comp_num);
    } else if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY) {
        ALLOC_ARRAY(v->ih_link_elfeat, EXPL_COMP *, v->comp_num);
    } else if (ap->inp_feat_mode == FEAT_MODE_BOTH) {
        ALLOC_ARRAY(v->ih_link_feat, EXPL_COMP **, v->comp_num);
        ALLOC_ARRAY(v->ih_link_elfeat, EXPL_COMP *, v->comp_num);
    } else {
        ASSERT(0);
    }
    ALLOC_ARRAY(v->ih_link_word, EXPL_COMP ***, v->comp_num);
    
    
    
    for (l = 0; l < v->comp_num; l++) {
        if (v->ih_links_specs[l].info_type == DEPREL_TYPE) {
            //+1 added: used when  constituent for which dependency is considered  is missing
            //+1 added for ROOT_DEPREL (it is not included in mp->deprel_num)
            ALLOC_ARRAY(v->ih_link_deprel[l], EXPL_COMP, mp->deprel_num + 2);
        } else if (v->ih_links_specs[l].info_type == POS_TYPE) {
            //+1 added: used when  constituent for which POS tag is considered  is missing
            ALLOC_ARRAY(v->ih_link_pos[l], EXPL_COMP, mp->pos_info_in.num + 1);
        } else if (v->ih_links_specs[l].info_type == WORD_TYPE) {
            ALLOC_ARRAY(v->ih_link_word[l], EXPL_COMP**, mp->pos_info_in.num + 1);
            int pos_idx, feat_idx;
            for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
                ALLOC_ARRAY(v->ih_link_word[l][pos_idx], EXPL_COMP*, mp->pos_info_in.feat_infos[pos_idx].num);
                for (feat_idx = 0; feat_idx < mp->pos_info_in.feat_infos[pos_idx].num; feat_idx++) {
                    ALLOC_ARRAY(v->ih_link_word[l][pos_idx][feat_idx], EXPL_COMP, mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num);
                }
            }
            //ALLOC item v->ih_link_word[l][mp->pos_info_in.num][0][0] for no-such word item
            ALLOC_ARRAY(v->ih_link_word[l][mp->pos_info_in.num], EXPL_COMP*, 1);
            ALLOC_ARRAY(v->ih_link_word[l][mp->pos_info_in.num][0], EXPL_COMP, 1);
            
        } else if (v->ih_links_specs[l].info_type == FEAT_TYPE) {
            if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                ALLOC_ARRAY(v->ih_link_feat[l], EXPL_COMP*, mp->pos_info_in.num + 1);
                int pos_idx;
                for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
                    ALLOC_ARRAY(v->ih_link_feat[l][pos_idx], EXPL_COMP, mp->pos_info_in.feat_infos[pos_idx].num);
                }
                //ALLOC item v->ih_link_feat[l][mp->pos_info_in.num][0] for no-such word item
                ALLOC_ARRAY(v->ih_link_feat[l][mp->pos_info_in.num], EXPL_COMP, 1);
            }
            if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                ALLOC_ARRAY(v->ih_link_elfeat[l], EXPL_COMP, mp->elfeat_num + 1); 
            }
        } else if (v->ih_links_specs[l].info_type == CPOS_TYPE) {
            ALLOC_ARRAY(v->ih_link_cpos[l], EXPL_COMP, mp->cpos_num + 1);
        } else if (v->ih_links_specs[l].info_type == LEMMA_TYPE) {
            ALLOC_ARRAY(v->ih_link_lemma[l], EXPL_COMP, mp->lemma_num + 1);
        } else {
            ASSERT(0);
        }
    }
}
#define get_pos_out_num() ((ap->input_offset > 0) ? mp->pos_info_out.num + ap->input_offset : mp->pos_info_out.num) 

void allocate_dd_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, DD_VECTOR *v) {

/*    out_link_act, 
        out_link_la_label, 
        out_link_ra_label, 
        out_link_pos,
        *out_link_feat, //[pos]
        **out_link_word; //[pos][feat]  */

    //allocate output weights
    ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
    ALLOC_ARRAY(v->out_link_feat, DD_COMP, mp->pos_info_out.num + 1);
    ALLOC_ARRAY(v->out_link_word, DD_COMP*, mp->pos_info_out.num + 1);
    
    int pos_idx, feat_idx;
    
    for (pos_idx = 0; pos_idx < mp->pos_info_out.num; pos_idx++) {
        ALLOC_ARRAY(v->out_link_word[pos_idx], DD_COMP,  mp->pos_info_out.feat_infos[pos_idx].num);
    }
    ALLOC_ARRAY(v->out_link_word[mp->pos_info_out.num], DD_COMP, 1); 

#define ALLOC_DD_COMP(ddc, out_num) {\
    ALLOC_ARRAY((ddc).b, double, (out_num))\
    ALLOC_ARRAY((ddc).ut, int, (out_num))\
    ALLOC_ARRAY((ddc).w, double*, (out_num))\
    int k;\
    for (k = 0; k < (out_num); k++){\
        ALLOC_ARRAY((ddc).w[k], double, (ap->hid_size));\
    }\
}  
    ALLOC_DD_COMP(v->out_link_act, ACTION_NUM);
    ALLOC_DD_COMP(v->out_link_la_label, mp->deprel_num + 1);
    ALLOC_DD_COMP(v->out_link_ra_label, mp->deprel_num + 1);
    ALLOC_DD_COMP(v->out_link_pos, get_pos_out_num());
    
    for (pos_idx = 0; pos_idx < mp->pos_info_out.num + 1; pos_idx++) { 
        ALLOC_DD_COMP(v->out_link_feat[pos_idx], mp->pos_info_out.feat_infos[pos_idx].num);
        for (feat_idx = 0; feat_idx < mp->pos_info_out.feat_infos[pos_idx].num; feat_idx++) {
            ALLOC_DD_COMP(v->out_link_word[pos_idx][feat_idx], 
                    mp->pos_info_out.feat_infos[pos_idx].word_infos[feat_idx].num);
        }        
    }
    
}

void allocate_comb_vector(APPROX_PARAMS *ap, MODEL_PARAMS *mp, COMB_VECTOR *v) {
    if (ap->use_expl_comp) {
        allocate_expl_vector(ap, mp, &(v->expl_vector));
    } 
    if (ap->use_dd_comp) {
        allocate_dd_vector(ap, mp, &(v->dd_vector));
    }

}


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

void loc_save_candidates(APPROX_PARAMS *ap, MODEL_PARAMS *mp, FILE *fp, SENTENCE **sents, double *scores, int avail_num) {
    if (!ap->return_cand) {
        return;
    }
    fprintf(fp, "%d\n\n", avail_num);
    int i;
    for (i = 0; i < avail_num; i++) {
        char *s = print_sent(sents[i], 1);
        fprintf(fp, "%e\n%s\n", scores[i], s);        
        free(s);
    } 
}


void test_loop(APPROX_PARAMS *ap, MODEL_PARAMS *mp, COMB_VECTOR *sum_x) {
    char inp_cand_fname[MAX_NAME];
    strcpy(inp_cand_fname, ap->out_file);
    strcat(inp_cand_fname, ".top");
    DEF_FOPEN(tfp, inp_cand_fname, "r");

    char out_cand_fname[MAX_NAME];
    strcpy(out_cand_fname, ap->out_file);
    strcat(out_cand_fname, ".top.rr");
    DEF_FOPEN(out_tfp, out_cand_fname, "w");     
    
    char out_fname[MAX_NAME];
    strcpy(out_fname, ap->out_file);
    strcat(out_fname, ".rr");
    DEF_FOPEN(ofp, out_fname, "w");     

    int is_blind_gld = is_blind_file(ap->test_file);

    DEF_FOPEN(gfp, ap->test_file, "r");
   
    
    SENTENCE *sents[MAX_BEAM];
    double scores[MAX_BEAM];
    double lprobs[MAX_BEAM], lprob_feats[MAX_BEAM];
    int matched = 0, tot = 0;
    OPTION_LIST *der_first[MAX_BEAM], *der_last[MAX_BEAM];
    SENTENCE *gld_sent;
    int ind_num, c = 0;
    printf("[Perceptron] Reranking...\n");
    int sum_x_ref;
    if (ap->use_dd_comp) {
        sum_x_ref = attach_dd_vector(ap, mp, &sum_x->dd_vector);
    }
    while ((ind_num = read_cand_num(tfp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }

        if (!is_blind_gld) {
            gld_sent = read_sentence(mp, gfp, 1);
        }
        
        int i;
        for (i = 0; i < ind_num; i++) {
            lprobs[i] = read_lprob(tfp);
            sents[i] = read_sentence(mp, tfp, 1);
            ASSERT(sents[i] != NULL);
        }

        double del = lprobs[ind_num - 1];
        for (i = 0; i < ind_num; i++) {
            lprobs[i] -= del;
        }
        comp_lprob_feats(ap, ind_num, lprobs, lprob_feats);    

        for(i = 0; i < ind_num; i++) {
            get_derivation_list(ap, mp, sents[i], &der_first[i], &der_last[i], 0);
            
            scores[i] = comp_score(ap, mp, sum_x, sum_x_ref, sents[i], lprobs[i], der_first[i], der_last[i]);
            free_derivation(der_last[i]); 
            der_last[i] = NULL;  der_first[i] = NULL;
        }   
    
        rearrange_scores(ap, ind_num, lprobs, scores);
        for(i = 0; i < ind_num; i++) { 
            scores[i] += lprob_feats[i] * sum_x->lprob_feat * ap->lprob_w;
        }
 
        int best_scored = -1;
        for(i = 0; i < ind_num; i++) {
            if (best_scored< 0 || scores[i] > scores[best_scored]) {
                best_scored = i;
            }
        }
        if (!is_blind_gld) {
            matched  += get_matched_syntax(sents[best_scored], gld_sent, 1);
            free(gld_sent);
        }
        
        tot += sents[0]->len;
        save_sentence(ofp, sents[best_scored], 1);
        loc_save_candidates(ap, mp, out_tfp, sents, scores, ind_num);
        
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
    
    printf("\n[Perceptron] Finished testing, accuracy : %f %%\n", ((double) matched) / tot);

    if (ap->use_dd_comp) {
        dettach_dd_vectors(ap, mp);
    }
    fclose(out_tfp);
    fclose(tfp);
    fclose(gfp);
    fclose(ofp);
}



void train_loop(APPROX_PARAMS *ap, MODEL_PARAMS *mp, COMB_VECTOR *curr_x, COMB_VECTOR *sum_x) {
    char inp_cand_fname[MAX_NAME];
    strcpy(inp_cand_fname, ap->model_name);
    strcat(inp_cand_fname, ".train.top");
    DEF_FOPEN(tfp, inp_cand_fname, "r");

    DEF_FOPEN(gfp, ap->train_file, "r");
    
    
    char aperc_file[MAX_NAME];
    strcpy(aperc_file, ap->model_name);
    strcat(aperc_file, ".av_perc");
    
    char cperc_file[MAX_NAME];
    strcpy(cperc_file, ap->model_name);
    strcat(cperc_file, ".cu_perc");
    
    
    SENTENCE *sents[MAX_BEAM];
    double scores[MAX_BEAM];
    double lprobs[MAX_BEAM], lprob_feats[MAX_BEAM];
    int matched[MAX_BEAM];
    OPTION_LIST *der_first[MAX_BEAM], *der_last[MAX_BEAM];
    SENTENCE *gld_sent;
    int ind_num, c = 0;
    printf("[Perceptron] Training...\n");

    //allocate dd comp
    DD_VECTOR *dd_vectors[MAX_BEAM];

    //id of reference from weigtmatrix to dd vector
    int dd_vectors_refs[MAX_BEAM], curr_x_ref, sum_x_ref;

    if (ap->use_dd_comp) {
        int i;
        for (i = 0; i < ap->cand_num; i++) {
            ALLOC(dd_vectors[i], DD_VECTOR);
            allocate_dd_vector(ap, mp, dd_vectors[i]);

            //attaches these vector to the weight matrix
            dd_vectors_refs[i] = attach_dd_vector(ap, mp, dd_vectors[i]);
        }
        curr_x_ref = attach_dd_vector(ap, mp, &curr_x->dd_vector);
        sum_x_ref = attach_dd_vector(ap, mp, &sum_x->dd_vector);
    }

    
    
    while ((ind_num = read_cand_num(tfp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }
        gld_sent = read_sentence(mp, gfp, 1);
        
        int i;
        for (i = 0; i < ind_num; i++) {
            lprobs[i] = read_lprob(tfp);
            sents[i] = read_sentence(mp, tfp, 1);
            ASSERT(sents[i] != NULL);
        }

        double del = lprobs[ind_num - 1];
        for (i = 0; i < ind_num; i++) {
            lprobs[i] -= del;
        }

        comp_lprob_feats(ap, ind_num, lprobs, lprob_feats);    
        
        int best_id = -1;
        for(i = 0; i < ind_num; i++) {
            matched[i] = get_matched_syntax(sents[i], gld_sent, 1);
            if (best_id < 0 || matched[i] > matched[best_id]) {
                best_id = i;
            }    

            get_derivation_list(ap, mp, sents[i], &der_first[i], &der_last[i], 0);
            
           
            scores[i] = comp_score_and_dd_vector(ap, mp, curr_x, curr_x_ref, sents[i], lprob_feats[i], der_first[i], der_last[i], dd_vectors[i], dd_vectors_refs[i]);
            
            free_derivation(der_last[i]); 
            der_last[i] = NULL; der_first[i] = NULL;
        }   
   
        rearrange_scores(ap, ind_num, lprobs, scores);
        for(i = 0; i < ind_num; i++) {
            scores[i] += lprob_feats[i] * curr_x->lprob_feat * ap->lprob_w;
        }
        
        ASSERT(best_id >= 0);
        
        int best_scored = -1;
        for(i = 0; i < ind_num; i++) {
            if (best_scored< 0 || scores[i] > scores[best_scored]) {
                best_scored = i;
            }
        }
        
        if (best_scored == best_id)  {
            printf("+");
            fflush(stdout);
        } else {
            printf("-");
            fflush(stdout);
            update_vectors(ap, mp, c,  ind_num, best_id, curr_x, curr_x_ref,  sum_x,  sum_x_ref, sents, matched, lprobs, lprob_feats, scores,  dd_vectors, dd_vectors_refs);
        }
        
      
        
        free(gld_sent);
        for (i = 0; i < ind_num; i++) {
            free(sents[i]);
            sents[i] = NULL;
        }

        
        fflush(stdout);
        c++;
    }
    final_update_vector(ap, mp, c, curr_x, sum_x);
   
    save_vector(ap, mp, aperc_file, sum_x);
    save_vector(ap, mp, cperc_file, curr_x);
    if (ap->use_dd_comp) {
        dettach_dd_vectors(ap, mp);
    }
    //TODO: free_dd_vectors
    
    fclose(tfp);
    fclose(gfp); 
    printf("\n[Perceptron] Finished training loop\n");
}

void process_rerank(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    read_ih_links_spec(ap->ih_links_spec_file, mp, mp->ih_links_specs, &mp->ih_links_num);
    read_hh_links_spec(ap->hh_links_spec_file, mp);
    allocate_weights(ap, mp);

    
    char best_weights_fname[MAX_NAME];
    strcpy(best_weights_fname, ap->model_name);
    strcat(best_weights_fname, ".best.wgt");
    load_weights(ap, mp, best_weights_fname);

    DEF_ALLOC(sv, COMB_VECTOR);
    char v_spec_fname[MAX_NAME];
    strcpy(v_spec_fname, ap->ih_links_spec_file);
    strcat(v_spec_fname, ".rerank");
   
    if (ap->use_expl_comp) { 
        read_ih_links_spec(v_spec_fname, mp, sv->expl_vector.ih_links_specs, &sv->expl_vector.comp_num);
    }
   
      
    
    allocate_comb_vector(ap, mp, sv);
    
  
    char aperc_file[MAX_NAME];
    strcpy(aperc_file, ap->model_name);
    strcat(aperc_file, ".av_perc");
    load_vector(ap, mp, aperc_file, sv);
    
    test_loop(ap, mp, sv);
    
}

void process_train(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    read_ih_links_spec(ap->ih_links_spec_file, mp, mp->ih_links_specs, &mp->ih_links_num);
    read_hh_links_spec(ap->hh_links_spec_file, mp);
    allocate_weights(ap, mp);

    
    char best_weights_fname[MAX_NAME];
    strcpy(best_weights_fname, ap->model_name);
    strcat(best_weights_fname, ".best.wgt");
    load_weights(ap, mp, best_weights_fname);

    DEF_ALLOC(v, COMB_VECTOR);
    DEF_ALLOC(sv, COMB_VECTOR);

    
    char v_spec_fname[MAX_NAME];
    strcpy(v_spec_fname, ap->ih_links_spec_file);
    strcat(v_spec_fname, ".rerank");
   
    if (ap->use_expl_comp) { 
        read_ih_links_spec(v_spec_fname, mp, v->expl_vector.ih_links_specs, &v->expl_vector.comp_num);
        read_ih_links_spec(v_spec_fname, mp, sv->expl_vector.ih_links_specs, &sv->expl_vector.comp_num);
    }
        
    allocate_comb_vector(ap, mp, v);
    allocate_comb_vector(ap, mp, sv);
    
    train_loop(ap, mp, v, sv);
    //DEBUG
    //test_loop(ap, mp, sv);
    
}

void process_rand(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    char cand_fname[MAX_NAME];
    strcpy(cand_fname, ap->out_file);
    strcat(cand_fname, ".top");

    char out_file[MAX_NAME];
    strcpy(out_file, ap->out_file);
    strcat(out_file, ".rand");
    
    DEF_FOPEN(tfp, cand_fname, "r");
    DEF_FOPEN(ofp, out_file, "w");
    DEF_FOPEN(gfp, ap->test_file, "r");
    
    SENTENCE *sents[ap->cand_num];
    SENTENCE *gld_sent;
    int ind_num, c = 0;
    printf("[Rand] Reranking...");
    int tot_matched = 0, tot_words = 0;
    while ((ind_num = read_cand_num(tfp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }
        gld_sent = read_sentence(mp, gfp, 1);
        tot_words += gld_sent->len;
        
        int i;
        for (i = 0; i < ind_num; i++) {
            read_lprob(tfp);
            sents[i] = read_sentence(mp, tfp, 1);
            ASSERT(sents[i] != NULL);
        }

        int j = (int) (drand48() * ind_num);
        tot_matched += get_matched_syntax(sents[j], gld_sent, 1);

        save_sentence(ofp, sents[j], 1);
        
        free(gld_sent);
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
    printf("Accuracy : %.5f %% \n", (100.0 * tot_matched) / tot_words);
    fclose(tfp);
    fclose(ofp);
    fclose(gfp);

}

void process_oracle(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
    char cand_fname[MAX_NAME];
    strcpy(cand_fname, ap->out_file);
    strcat(cand_fname, ".top");

    char out_file[MAX_NAME];
    strcpy(out_file, ap->out_file);
    strcat(out_file, ".oracle");
    
    DEF_FOPEN(tfp, cand_fname, "r");
    DEF_FOPEN(ofp, out_file, "w");
    DEF_FOPEN(gfp, ap->test_file, "r");
    
    SENTENCE *sents[ap->cand_num];
    SENTENCE *gld_sent;
    int ind_num, c = 0;
    printf("[Oracle] Reranking...");
    int tot_matched = 0, tot_words = 0;
    while ((ind_num = read_cand_num(tfp) ) > 0) {
        if (ind_num > ap->cand_num) {
            fprintf(stderr, "Error: Number of candiates in file (%d) in file exceeded maximum (CAND_NUM = %d)\n", ind_num, ap->cand_num);
            exit(1);
        }
        gld_sent = read_sentence(mp, gfp, 1);
        tot_words += gld_sent->len;
        
        int i;
        for (i = 0; i < ind_num; i++) {
            read_lprob(tfp);
            sents[i] = read_sentence(mp, tfp, 1);
            ASSERT(sents[i] != NULL);
        }


        int max_matched = -1; int best_id = -1;
        for(i = 0; i < ind_num; i++) {
            int matched = get_matched_syntax(sents[i], gld_sent, 1);
            
            if (matched > max_matched) {
                max_matched = matched;
                best_id = i;
            }    
        }   
        
        ASSERT(best_id >= 0);
        save_sentence(ofp, sents[best_id], 1);
        tot_matched += max_matched;
        
        free(gld_sent);
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
    printf("Accuracy : %.5f %% \n", (100.0 * tot_matched) / tot_words);
    fclose(tfp);
    fclose(ofp);
    fclose(gfp);
}


int print_usage() {
    fprintf(stderr, "Usage: ./idp_reranker (-oracle|-train|-rand|-rerank) settings_file [ADDITIONAL_PARAMETER=VALUE]*\n");
    fprintf(stderr,"\t-oracle\toracle prediction (TEST_FILE should point to a file with gold std predictions) \n");
    fprintf(stderr,"\t-train\ttrains the reranker\n");
    fprintf(stderr,"\t-rerank\treranks the test data\n");
    fprintf(stderr,"\t-rand\tuniform prob prediction\n");
    fprintf(stderr,"Additional parameters are used to change/add parameters defined in settings_file\n");
    exit(1);
}


int main(int argc, char **argv) {
    printf("Reranker for Dependency Parsing\n");
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
    
    srand48(ap->seed);
    
    if (strcmp(mode, "-oracle") == 0) {
        process_oracle(ap, mp);
    } else if (strcmp(mode, "-train") == 0) {
        process_train(ap, mp);        
    } else if (strcmp(mode, "-rand") == 0) {
        process_rand(ap, mp);
    } else if (strcmp(mode, "-rerank") == 0) {
        process_rerank(ap, mp);
    } else {
        fprintf(stderr, "Error: mode '%s' is not supported\n", mode);
        print_usage();
    }
    
    free(ap);
    free(mp);
    mp = NULL;
    return 0; 
}    

