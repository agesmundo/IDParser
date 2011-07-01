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

#define SMALL_POS_VAL 1.e-12
#define PRUNE_FACT 0.5
#define QUEUE_SIZE_LIMIT 3000

#define get_pos_out_num() ((ap->input_offset > 0) ? mp->pos_info_out.num + ap->input_offset : mp->pos_info_out.num) 

OPTION *root_prediction(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent) {

    //opt to predict shift
    STACK st; st.size = 0;
    STACK srl_st; srl_st.size = 0;
    OPTION *opt = create_option(SRL_STEP, 1, -1, -1, NULL, 1, &st, &srl_st, 1, pt_alloc(sent->len), 
            ps_alloc(sent->len), &mp->out_link_act, ACTION_NUM);
    set_links(ap, mp, opt, sent);
    return opt;
}

double max_word_pred_lprob(APPROX_PARAMS *ap, 
        MODEL_PARAMS *mp,  SENTENCE *s, OPTION **new_b_options, int new_cand_num, OPTION *opt) {
    //TODO implement computation to speed up the process
    if (ap->beam == new_cand_num) {
        int i;
        double max_prev = - MAXDOUBLE;
        for (i = 0; i < ap->beam; i++) {
            double pred_lprob = new_b_options[i]->lprob - new_b_options[i]->previous_option
                ->previous_option->previous_option->lprob;
            if (pred_lprob > max_prev) {
                max_prev = pred_lprob;
            }
        }
        return max_prev * PRUNE_FACT;
    }
    return 0;
}
double min_word_pred_lprob(APPROX_PARAMS *ap, 
        MODEL_PARAMS *mp,  SENTENCE *s, OPTION **new_b_options, int new_cand_num, OPTION *opt) {
    //TODO implement computation to speed up the process
    if (ap->beam == new_cand_num) {
        int i;
        double min_prev = MAXDOUBLE;
        for (i = 0; i < ap->beam; i++) {
            double pred_lprob = new_b_options[i]->lprob - new_b_options[i]->previous_option
                ->previous_option->previous_option->lprob;
            if (pred_lprob < min_prev) {
                min_prev = pred_lprob;
            }
        }
        return min_prev;
    }
    return 0;
}

//TODO implement efficiently
int get_best_labels(APPROX_PARAMS *ap, OPTION *opt, double min_dec_prob, int lab_num, int *best_rels) {
    int fan = 0;
    DEF_ALLOC_ARRAY(spare, int, lab_num);
    int i, j;
    for (j = 0; j < ap->search_br_factor; j++) {
        int max_i = -1;
        for (i = 0; i < lab_num; i++) {
            if (spare[i] || opt->outputs.q[i] < min_dec_prob) {
                continue;
            }     
            if (max_i < 0 || opt->outputs.q[i] > opt->outputs.q[max_i]) {
                max_i = i;
            }
        }
        if (max_i >= 0) {
            best_rels[fan++] = max_i;
            spare[max_i] = 1;
        } else {
            break;
        }
    } 
    free(spare);
    return fan;
}

void add_to_best_list(APPROX_PARAMS *ap, OPTION **new_b_options, int *new_cand_num, OPTION *to_add) {
    ASSERT(*new_cand_num < ap->beam || to_add->lprob > new_b_options[ap->beam - 1]->lprob);
    
    int i; 
    for (i = *new_cand_num - 1; i >= 0; i--) {
        if (new_b_options[i]->lprob < to_add->lprob) {
            new_b_options[i + 1] = new_b_options[i];
        } else {
            new_b_options[i + 1] = to_add;
            break;
        }
    }
    //should be inserted on the first position 
    if (i == -1) {
        new_b_options[0] = to_add;
    }
    if (*new_cand_num == ap->beam) {
        pt_free(&(new_b_options[ap->beam]->pt));
        ps_free(&(new_b_options[ap->beam]->ps));
        free_options(new_b_options[ap->beam]);
    } else {
        (*new_cand_num)++;
    }
    return;         
    
}

//TODO don't forget to move pt
OPTION_LIST *process_opt(APPROX_PARAMS *ap, MODEL_PARAMS *mp,  SENTENCE *sent, OPTION **new_b_options, int *new_cand_num, 
        OPTION *opt, OPTION_LIST *ol) {

    /* if ol longer than QUEUE_SIZE_LIMIT, then prune to half that length.  
       Requires calling with opt=NULL to initialize count */
    static int queue_size;
    if (opt == NULL) {
      queue_size = 0;
      OPTION_LIST *ol2;
      for (ol2 = ol; ol2 != NULL; ol2 = ol2->prev)
	queue_size++;
      return ol;
    }
    queue_size--;

    ASSERT(opt->outputs.num == ACTION_NUM);
    
    double min_lprob = - MAXDOUBLE;
    double min_lprob_with_pred = -MAXDOUBLE;
    if (*new_cand_num == ap->beam) {
        min_lprob = new_b_options[ap->beam - 1]->lprob  - max_word_pred_lprob(ap, mp, sent, new_b_options, *new_cand_num,  opt);
        min_lprob_with_pred = new_b_options[ap->beam - 1]->lprob;
    }
    
    //do not need to consider
    if (opt->lprob < min_lprob) {
       pt_free(&(opt->pt));
       ps_free(&(opt->ps));
       free_options(opt);
        return ol;
    }
    //they are not yet computed
    ACTION_SET as = get_next_actions(ap, mp, opt, sent);
    set_mask(mp, opt, &as);
    comp_option(ap, mp, opt);

    // -----------------------  SYNTAX -----------------------------------------------

    //check SHIFT (first because it is the operation that updates candidate list)
    if (as.acts[SHIFT]) {
        if (log(opt->outputs.q[SHIFT]) + opt->lprob >= min_lprob) {
            //create sequence of shift operation
            //update queue and stack
            //TODO Implement more efficiently pruning after each decision
            OPTION *as_opt = create_option(SYNT_STEP, 0, opt->previous_act, SHIFT, opt, opt->period, &opt->stack, 
                    &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_pos, 
                    get_pos_out_num());
            int pos = sent->pos[opt->period + ap->input_offset];
            int feat = sent->feat_out[opt->period + ap->input_offset];
            int word = sent->word_out[opt->period + ap->input_offset];
            set_links(ap, mp, as_opt, sent);

            comp_option(ap, mp, as_opt);
           
            if (log(as_opt->outputs.q[pos]) + as_opt->lprob < min_lprob_with_pred) {
                pt_free(&(as_opt->pt));
                ps_free(&(as_opt->ps));
                free_option(as_opt);
            } else {
            
                OPTION *last_opt = create_word_prediction_seq(ap, mp, sent, as_opt, 1);
                OPTION *feat_opt = last_opt->previous_option->previous_option;
                
                comp_option(ap, mp, feat_opt);
                if (log(feat_opt->outputs.q[feat]) + feat_opt->lprob < min_lprob_with_pred) {
                    pt_free(&(last_opt->pt));
                    ps_free(&(last_opt->ps));
                    OPTION *po = last_opt->previous_option, *ppo = last_opt->previous_option->previous_option;
                    free_option(last_opt); 
                    free_option(po);
                    free_option(ppo);
                    free_option(as_opt);
                } else {
                    OPTION *word_opt = last_opt->previous_option;

                    comp_option(ap, mp, word_opt);
//                    if (log(word_opt->outputs.q[word]) + word_opt->lprob < min_lprob_with_pred) {
                    if (log(word_opt->outputs.q[word]) + word_opt->lprob <  min_lprob_with_pred + SMALL_POS_VAL) {
                        pt_free(&(last_opt->pt));
                        ps_free(&(last_opt->ps));
                        OPTION *po = last_opt->previous_option, *ppo = last_opt->previous_option->previous_option;
                        free_option(last_opt); 
                        free_option(po); 
                        free_option(ppo);
                        free_option(as_opt);
                    } else {
                        fill_lprob(last_opt);                            
                        add_to_best_list(ap, new_b_options, new_cand_num, last_opt);
                    }
                }
            }

        }
    }

    //update
    if (*new_cand_num == ap->beam) {
        min_lprob = new_b_options[ap->beam - 1]->lprob  - max_word_pred_lprob(ap, mp, sent, new_b_options, *new_cand_num,  opt);
        min_lprob_with_pred = new_b_options[ap->beam - 1]->lprob;
	// not necessary, but could save space:
	//ol = prune_options(ol, min_lprob);
    }
          
    //check LA
    if (as.acts[LA]) {
        int d = st_peek(&opt->stack);     
        int h = opt->queue;
        if (log(opt->outputs.q[LA]) + opt->lprob >= min_lprob) {
            //create sequence of LA operations
            //update queue and stack
            OPTION *as_opt = create_option(SYNT_STEP, 0, opt->previous_act, LA, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_la_label, mp->deprel_num);
            set_links(ap, mp, as_opt, sent);        
            comp_option(ap, mp, as_opt);

            double deprel_min_prob = exp(min_lprob - as_opt->lprob);
            
            int best_rels[MAX_DEPREL_SIZE];
            int fanout = get_best_labels(ap,  as_opt, deprel_min_prob, mp->deprel_num, best_rels);
            int i;
            for (i = 0; i < fanout; i++) {
                OPTION *new_opt = create_option_light(SYNT_STEP, 1, LA, best_rels[i], as_opt, as_opt->period, &as_opt->stack, 
                        &as_opt->srl_stack, as_opt->queue, pt_clone(as_opt->pt), ps_clone(as_opt->ps), &mp->out_link_act, ACTION_NUM);
                pt_add_link(new_opt->pt, h, d, best_rels[i]);
                if (ap->intern_synt_deproj == DEPROJ_NO) {
                    st_pop(&new_opt->stack);
                } else {
                    // Do nothing
                }
                set_links(ap, mp, new_opt, sent);        
                fill_lprob(new_opt);
                ol = increase_list_asc(ol, new_opt);
		        queue_size++;
            } 
            pt_free(&(as_opt->pt));
            ps_free(&(as_opt->ps));
            as_opt->pt = NULL; as_opt->ps = NULL;
            if (fanout == 0) {
                free_option(as_opt);
            }
        }
    } 
    
    //check RA
    if (as.acts[RA]) {
        int d = opt->queue;
        int h = st_peek(&opt->stack);
        //when agrees
        if (log(opt->outputs.q[RA]) + opt->lprob >= min_lprob) {
            //create sequence of RA operations                
            //update queue and stack
            OPTION *as_opt = create_option(SYNT_STEP, 0, opt->previous_act, RA, opt, opt->period, &opt->stack, &opt->srl_stack, 
                    opt->queue, pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_ra_label, mp->deprel_num);
            set_links(ap, mp, as_opt, sent);
            comp_option(ap, mp, as_opt);

            double deprel_min_prob = exp(min_lprob - as_opt->lprob);
                
            int best_rels[MAX_DEPREL_SIZE];
            int fanout = get_best_labels(ap,  as_opt, deprel_min_prob, mp->deprel_num, best_rels);
            int i;
            for (i = 0; i < fanout; i++) {
                OPTION *new_opt = create_option_light(SYNT_STEP, 1, RA, best_rels[i], as_opt, as_opt->period, &as_opt->stack, &as_opt->srl_stack, 
                        as_opt->queue, pt_clone(as_opt->pt), ps_clone(as_opt->ps), &mp->out_link_act, 
                    ACTION_NUM);
                pt_add_link(new_opt->pt, h, d, best_rels[i]);
                set_links(ap, mp, new_opt, sent);        
                fill_lprob(new_opt);
                ol = increase_list_asc(ol, new_opt);
		queue_size++;
            }
            pt_free(&(as_opt->pt));
            ps_free(&(as_opt->ps));
            as_opt->pt = NULL; as_opt->ps = NULL;
            if (fanout == 0) {
                free_option(as_opt);
            }
            //TODO Try processing immediately SHIFT operation    
            //should speed-up (it will make it way to best_list and 
            //prune other optinos)
        }
    }
    //check RED
    if (as.acts[RED]) {
        if (log(opt->outputs.q[RED]) + opt->lprob >= min_lprob) {
        
            //create sequence of reduce operation
            //update queue and stack
            OPTION *as_opt = create_option_light(SYNT_STEP, 1, RED, RED, opt, opt->period, &opt->stack, &opt->srl_stack, 
                    opt->queue, pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act,
                    ACTION_NUM);
            st_pop(&as_opt->stack);
            set_links(ap, mp, as_opt, sent);
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
	    queue_size++;

        }
    }
     //check SWITCH
    if (as.acts[SWITCH]) {
        if (log(opt->outputs.q[SWITCH]) + opt->lprob >= min_lprob) {
            OPTION *as_opt = create_option_light(SRL_STEP, 1, SWITCH, SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
//            OPTION *as_opt = create_option_light(SYNT_STEP, 1, SWITCH, SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
//                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
            
            set_links(ap, mp, as_opt, sent);
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
	    queue_size++;
        }
    }
    
    // -----------------------  SEMANTICS -----------------------------------------------
    //check PREDIC_NO
    if (as.acts[PREDIC_NO]) {
        if (log(opt->outputs.q[PREDIC_NO]) + opt->lprob >= min_lprob) {
            OPTION *as_opt = create_option_light(SRL_STEP, 1, PREDIC_NO, PREDIC_NO, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
            
            set_links(ap, mp, as_opt, sent);
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
	    queue_size++;
        }
    }

    if (as.acts[PREDIC_YES]) {
        int q = opt->queue;
        int q_bank = sent->bank[q];
        int q_lemma = sent->lemma[q];
        
        if (log(opt->outputs.q[PREDIC_YES]) + opt->lprob >= min_lprob) {
            //create sequence of predicate prediction operations                
            OPTION *as_opt = create_option(SRL_STEP, 0, opt->previous_act, PREDIC_YES, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt),  ps_clone(opt->ps),
                        &mp->out_link_sense[q_bank][q_lemma][0], mp->sense_num[q_bank][q_lemma]);
            set_links(ap, mp, as_opt, sent);
            comp_option(ap, mp, as_opt);
            
            double sense_min_prob = exp(min_lprob - as_opt->lprob);
            int best_senses[MAX_SENSE_SIZE];
            int fanout = get_best_labels(ap, as_opt, sense_min_prob, mp->sense_num[q_bank][q_lemma], best_senses);
            int i;
            for (i = 0; i < fanout; i++) {
                OPTION *new_opt =  create_option_light(SRL_STEP, 1, PREDIC_YES, best_senses[i], as_opt, as_opt->period, &as_opt->stack,  &as_opt->srl_stack, as_opt->queue, pt_clone(as_opt->pt), ps_clone(as_opt->ps),
                    &mp->out_link_act, ACTION_NUM);
                ps_set_sense(new_opt->ps, q, best_senses[i]);
                set_links(ap, mp, new_opt, sent);        
                fill_lprob(new_opt);
                ol = increase_list_asc(ol, new_opt);
		queue_size++;
            }
            pt_free(&(as_opt->pt));
            ps_free(&(as_opt->ps));
            as_opt->pt = NULL; as_opt->ps = NULL;
            if (fanout == 0) {
                free_option(as_opt);
            }
        }
    }
    
    //check SRL_LA
    int q_bank = sent->bank[opt->queue];
    if (q_bank >= 0 && as.acts[SRL_LA[q_bank]]) {
        int d = st_peek(&opt->srl_stack);     
        int h = opt->queue;
        if (log(opt->outputs.q[SRL_LA[q_bank]]) + opt->lprob >= min_lprob) {

        //create sequence of SRL_LA operations
            //update queue and stack
            OPTION *as_opt  = create_option(SRL_STEP, 0, opt->previous_act, SRL_LA[q_bank], opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
                    &mp->out_link_sem_la_label[q_bank], mp->role_num[q_bank]);
            set_links(ap, mp, as_opt, sent);        
            comp_option(ap, mp, as_opt);

            double rel_min_prob = exp(min_lprob - as_opt->lprob);
            int best_rels[MAX_ROLE_SIZE];
            int fanout = get_best_labels(ap, as_opt, rel_min_prob, mp->role_num[q_bank], best_rels);
            int i;
            for (i = 0; i < fanout; i++) {
                OPTION *new_opt = create_option_light(SRL_STEP, 1, SRL_LA[q_bank], best_rels[i], as_opt, as_opt->period, &as_opt->stack, &as_opt->srl_stack, as_opt->queue, pt_clone(as_opt->pt), ps_clone(as_opt->ps),
                    &mp->out_link_act, ACTION_NUM);
                ps_add_link(new_opt->ps, h, d, best_rels[i]);
                set_links(ap, mp, new_opt, sent);        
                fill_lprob(new_opt);
                ol = increase_list_asc(ol, new_opt);
		queue_size++;
            }
            pt_free(&(as_opt->pt));
            ps_free(&(as_opt->ps));
            as_opt->pt = NULL; as_opt->ps = NULL;
            if (fanout == 0) {
                free_option(as_opt);
            }
        }
    } 
    q_bank = -1;

    //check SRL_RA
    int s_bank = sent->bank[st_peek(&opt->srl_stack)];
    if (s_bank >= 0 && as.acts[SRL_RA[s_bank]]) {
        int d = opt->queue;
        int h = st_peek(&opt->srl_stack);
        if (log(opt->outputs.q[SRL_RA[s_bank]]) + opt->lprob >= min_lprob) {
            //create sequence of RA operations                
            //update queue and stack
            OPTION *as_opt = create_option(SRL_STEP, 0, opt->previous_act, SRL_RA[s_bank], opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt),  ps_clone(opt->ps),
                        &mp->out_link_sem_ra_label[s_bank], mp->role_num[s_bank]);
            set_links(ap, mp, as_opt, sent);
            comp_option(ap, mp, as_opt);

            double rel_min_prob = exp(min_lprob - as_opt->lprob);
            int best_rels[MAX_ROLE_SIZE];
            int fanout = get_best_labels(ap, as_opt, rel_min_prob, mp->role_num[s_bank], best_rels);
            int i;
            for (i = 0; i < fanout; i++) {
                OPTION *new_opt = create_option_light(SRL_STEP, 1, SRL_RA[s_bank],  best_rels[i], as_opt, as_opt->period, &as_opt->stack,  &as_opt->srl_stack, as_opt->queue, pt_clone(as_opt->pt), ps_clone(as_opt->ps),
                        &mp->out_link_act, ACTION_NUM);
                ps_add_link(new_opt->ps, h, d, best_rels[i]);
                set_links(ap, mp, new_opt, sent);        
                fill_lprob(new_opt);
                ol = increase_list_asc(ol, new_opt);
		queue_size++;
            }
            pt_free(&(as_opt->pt));
            ps_free(&(as_opt->ps));
            as_opt->pt = NULL; as_opt->ps = NULL;
            if (fanout == 0) {
                free_option(as_opt);
            }
        }
    }
    s_bank = -1;

    //check SRL_RED
    if (as.acts[SRL_RED]) {
        if (log(opt->outputs.q[SRL_RED]) + opt->lprob >= min_lprob) {
            //create sequence of reduce operation
            //update queue and stack
            OPTION *as_opt = create_option_light(SRL_STEP, 1, SRL_RED, SRL_RED, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
            st_pop(&as_opt->srl_stack);
            set_links(ap, mp, as_opt, sent);
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
	    queue_size++;
        }
    }

    //check SRL_SWITCH
    if (as.acts[SRL_SWITCH]) {
        if (log(opt->outputs.q[SRL_SWITCH]) + opt->lprob >= min_lprob) {
            OPTION *as_opt = create_option_light(SYNT_STEP, 1, SRL_SWITCH, SRL_SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
            set_links(ap, mp, as_opt, sent);    
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
	    queue_size++;
        }
    }
    
    if (as.acts[SRL_FLIP]) {
        if (log(opt->outputs.q[SRL_FLIP]) + opt->lprob >= min_lprob) {
            OPTION *as_opt = create_option_light(SRL_STEP, 1, SRL_FLIP, SRL_FLIP, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, 
                    pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
            int srl_s = st_pop(&as_opt->srl_stack);
            int srl_s_under = st_pop(&as_opt->srl_stack);
            st_push(&as_opt->srl_stack, srl_s);
            st_push(&as_opt->srl_stack, srl_s_under);

            set_links(ap, mp, as_opt, sent);
            fill_lprob(as_opt);
            ol = increase_list_asc(ol, as_opt);
            queue_size++;

        }
    }

    pt_free(&(opt->pt));
    ps_free(&(opt->ps));
    opt->pt = NULL; opt->ps = NULL;
    if (opt->next_options->prev == NULL) {
        free_options(opt);
    }
    if (queue_size > QUEUE_SIZE_LIMIT) {
      printf("Warning: indiscriminately pruning search queue from length %d to %d\n", 
	     queue_size, (int) (QUEUE_SIZE_LIMIT / 1.5));
      OPTION_LIST *ol2;
      int cnt = 0;
      for (ol2 = ol; ol2 != NULL; ol2 = ol2->prev) {
	cnt++;
	if (cnt > (int) (QUEUE_SIZE_LIMIT / 1.5)) { // reduce to 2/3 of limit
	  prune_options(ol2, 0.0);
	  break;
	}
      }
      queue_size = cnt - 1;
    }
    return ol;
}

int search_options(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, OPTION **prev_b_options, int prev_cand_num, OPTION **new_b_options) {
    OPTION_LIST *ol = increase_list(NULL, NULL);
    int i;
    for (i = 0; i < prev_cand_num; i++) {
        increase_list_asc(ol, prev_b_options[i]);
    }

    int new_cand_num = 0;
    // to set static variable
    process_opt(ap, mp, sent, new_b_options, &new_cand_num, NULL, ol);
    while(ol->prev != NULL) {
        OPTION *opt = ol->prev->elem;
        ol = reduce_list(ol);                
        process_opt(ap, mp, sent, new_b_options, &new_cand_num, opt, ol);
    }
    free(ol);
    return new_cand_num;
}

double label_sentence(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent) {
    ASSERT(ap->beam >= 1 && ap->beam < MAX_BEAM);    
    ASSERT(ap->search_br_factor >= 1);
    

    OPTION  *best_options[MAX_SENT_LEN + 1][MAX_BEAM + 1];

    best_options[0][0] = root_prediction(ap, mp, sent);
    int cand_num = 1; 

    int t;
    for (t = 1; t < sent->len + 1; t++) {
        cand_num = search_options(ap, mp, sent, best_options[t-1], cand_num, best_options[t]);
        ASSERT(cand_num <= ap->beam);
    }
    double best_lprob = best_options[sent->len][0]->lprob;     
    pt_fill_sentence(best_options[sent->len][0]->pt, mp, sent);
    ps_fill_sentence(best_options[sent->len][0]->ps, mp, sent);

    save_candidates(ap, mp, sent, best_options[sent->len], cand_num);
    
    int i;
    for (i = 0; i < cand_num; i++) {
        pt_free(&(best_options[sent->len][i]->pt));
        ps_free(&(best_options[sent->len][i]->ps));
        free_options(best_options[sent->len][i]);
    }
    return best_lprob;
}
