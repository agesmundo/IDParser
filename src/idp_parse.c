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
const int SRL_LA[2] = {9, 10}; //prob bank and nombank left arc
const int SRL_RA[2] = {11, 12}; // prob bank and nombank right arc

#define IS_NOW_SYNTAX(prev_act)  ((prev_act) == SHIFT  ||  (prev_act) == LA || \
	(prev_act) == RA || (prev_act) == RED || (prev_act) == FLIP)


/** returns next possible actions */
ACTION_SET get_next_actions(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *sent) {
	ACTION_SET as;
	bzero(&as, sizeof(ACTION_SET));

	int s = st_peek(&opt->stack);
	ASSERT(opt->queue != opt->pt->len + 1 && opt->queue != opt->ps->len + 1);


	// if we just started we should decide on the predicate
	if (opt->previous_act < 0) {
		int bank = sent->bank[opt->queue];
		int lemma = sent->lemma[opt->queue];
		if(USE_FILLPRED==0){
			as.acts[PREDIC_NO] = 1;
			as.acts[PREDIC_YES] = (bank  >= 0 && mp->sense_num[bank][lemma] > 0);
		}
		else{
			if(sent->fill_pred[opt->queue]==1 && (bank  >= 0 && mp->sense_num[bank][lemma] > 0)){
				as.acts[PREDIC_YES]=1;
				as.acts[PREDIC_NO] =0;
			}
			else{
				as.acts[PREDIC_YES]=0;
				as.acts[PREDIC_NO] =1;
			}
		}
	} else if (opt->previous_act == SRL_SWITCH) {
		as.acts[SHIFT] = 1;
		// to support RA of first word
		if (ap->parsing_mode >= 3 && s == 0) {
			as.acts[RA] = 1;
		}
		// if we are still in syntactic part  *
	} else if (IS_NOW_SYNTAX(opt->previous_act)) {

		//RA is always followed by SHIFT
		if (opt->previous_act == RA && ap->intern_synt_deproj == DEPROJ_NO) {
			as.acts[RED] = 0;
			as.acts[LA] = 0;
			as.acts[RA] = 0;
			as.acts[SWITCH] = 1;
		} else {
			//empty stack
			if (s == 0) {
				as.acts[RED] = 0;
				as.acts[LA] = 0;
				//allows to attach to root
				if (ap->parsing_mode >= 3) {
					as.acts[RA] = 1;
				} else {
					as.acts[RA] = 0;
				}
				as.acts[SWITCH] = 1;
			} else {
				//reduce is not allowed for root-attached nodes with deprel other than root_deprel
				//TODO: Doubts in this condition. According to MALT in CoNLL doc they should be not allowed to reduce
				//but that won't allow:
				//1 A ... 0 X
				//2 B ... 0 X
				//because B can not be attached to root with RA (A is not removed from stack)
				//for parsing mode 4 - we allow to reduce elemenets attached to root from the stack
				//
				if (ap->parsing_mode == 3 &&  pt_head(opt->pt, s) == 0 && pt_deprel(opt->pt, s) == ROOT_DEPREL) {
					as.acts[RED] = 0;
				} else {
					as.acts[RED] = 1;
				}

				//if head is not assigned to s
				if (opt->pt->nodes[s].head == 0 && opt->pt->nodes[s].deprel == ROOT_DEPREL) {
					as.acts[LA] = 1;
				} else {
					as.acts[LA] = 0;
				}
				if (ap->intern_srl_deproj == DEPROJ_NO) {
					//  RA cannot be repeated because words are switched to stack
					//  in non-projective mode it can be flipped
					as.acts[RA] = 1;
				} else {
					if (opt->pt->nodes[opt->queue].head == 0 && opt->pt->nodes[opt->queue].deprel == ROOT_DEPREL) {
						as.acts[RA] = 1;
					} else {
						as.acts[RA] = 0;
					}
				}
				as.acts[SWITCH] = 1;
			}

		} //!RA
		//if the stack contains at least 2 elements
		if (ap->intern_synt_deproj > DEPROJ_NO &&  st_look(&opt->stack, 1) != 0 && opt->previous_act != FLIP) {
			as.acts[FLIP] = 1;
		}

		// Now semantics
		// First predicate prediction
	} else if (opt->previous_act == SWITCH) {
		int bank = sent->bank[opt->queue];
		int lemma = sent->lemma[opt->queue];
		if(USE_FILLPRED==0){
			as.acts[PREDIC_NO] = 1;
			as.acts[PREDIC_YES] = (bank  >= 0 && mp->sense_num[bank][lemma] > 0);
		}
		else{
			if(sent->fill_pred[opt->queue]==1 && (bank  >= 0 && mp->sense_num[bank][lemma] > 0)){
				as.acts[PREDIC_YES]=1;
				as.acts[PREDIC_NO] =0;
			}
			else{
				as.acts[PREDIC_YES]=0;
				as.acts[PREDIC_NO] =1;
			}
		}
		// semantics operations
	} else {
		//top of the srl stacl and front of queue
		int s_srl = st_peek(&opt->srl_stack);
		int q = opt->queue;

		//we always can switch to synt parsing
		as.acts[SRL_SWITCH] = 1;

		//empty stack
		if (s_srl != 0) {
			///if (s_srl >= 0) {
			//we can reduce from stack
			as.acts[SRL_RED] = 1;

			//LA opers
			int q_bank = sent->bank[q];
			int q_sense = ps_sense(opt->ps, q);

			// if q is a predicate
			if (q_bank >= 0 && q_sense >= 0
					// and not already a parent of s_srl
					&& !ps_is_head_of(opt->ps, q, s_srl)) {

				as.acts[SRL_LA[q_bank]] = 1;
			}

			//RA opers
			int s_bank = sent->bank[s_srl];
			int s_sense = ps_sense(opt->ps, s_srl);
			// if s_srl  is a predicate
			if (s_bank >= 0 && s_sense >= 0
					// and not already a parent of q
					&& !ps_is_head_of(opt->ps, s_srl, q)) {

				as.acts[SRL_RA[s_bank]] = 1;
			}

			//if the stack contains at least 2 elements
			if ( ap->intern_srl_deproj > DEPROJ_NO &&  st_look(&opt->srl_stack, 1) != 0 && opt->previous_act != SRL_FLIP) {
				as.acts[SRL_FLIP] = 1;
			}


		}  // STACK not empty
	}

	return as;
}


/* create_option, but without output array allocation */
OPTION *create_option_light(int step_type, int new_state, int previous_act, int previous_output, OPTION *previous_option, int period, STACK *stack,  STACK *srl_stack,
		int queue, PART_TREE *pt,  PART_SRL *ps,  OUT_LINK *out_link, int out_num) {
	DEF_ALLOC(opt, OPTION);
	opt->step_type = step_type;
	opt->new_state = new_state;
	opt->previous_act = previous_act;
	opt->previous_output = previous_output;
	opt->previous_option = previous_option;
	opt->period = period;
	memcpy(&opt->stack, stack, sizeof(STACK));
	memcpy(&opt->srl_stack, srl_stack, sizeof(STACK));
	opt->queue = queue;
	opt->pt = pt;
	opt->ps = ps;

	opt->next_options = increase_list(NULL, NULL);
	if  (previous_option != NULL) {
		previous_option->next_options
		= increase_list(previous_option->next_options, opt);

	}

	opt->out_link = out_link;
	opt->outputs.num = out_num;

	opt->outputs.mask = NULL;
	opt->outputs.q = NULL;
	opt->outputs.del = NULL;
	opt->outputs.parts = NULL;
	opt->outputs.exp = NULL;
	opt->outputs.a = NULL;
	opt->outputs.b = NULL;

	return opt;
}

void alloc_option_outputs(OPTION *opt) {
	int out_num = opt->outputs.num;
	ALLOC_ARRAY(opt->outputs.mask, double, out_num);
	ALLOC_ARRAY(opt->outputs.q, double, out_num);
	ALLOC_ARRAY(opt->outputs.del, double, out_num);
	ALLOC_ARRAY(opt->outputs.parts, double, out_num);
	ALLOC_ARRAY(opt->outputs.exp, double, out_num);
	ALLOC_ARRAY(opt->outputs.a, double, out_num);
	ALLOC_ARRAY(opt->outputs.b, double, out_num);
}

OPTION *create_option(int step_type, int new_state, int previous_act, int previous_output, OPTION *previous_option, int period, STACK *stack,  STACK *srl_stack,
		int queue, PART_TREE *pt,  PART_SRL *ps,  OUT_LINK *out_link, int out_num) {

	OPTION *opt =
		create_option_light(step_type, new_state, previous_act, previous_output,
				previous_option, period, stack, srl_stack, queue, pt, ps,
				out_link, out_num);
	alloc_option_outputs(opt);

	return opt;
}

/* return the (numprev+1)th SRL that matches sent,h,d, or -1 if none */
int next_srl_role(int numprev, SENTENCE *sent, int h, int d) {
	int a, i;
	i = numprev;
	for (a = 0; a < sent->arg_num[h]; a++) {
		if (sent->args[h][a] == d) {
			if (i == 0)
				return sent->arg_roles[h][a];
			else
				i--;
		}
	}
	return -1;
}
/* is r an SRL for sent,h,d? */
int is_srl_role(SENTENCE *sent, int h, int d, int r) {
	int a;
	for (a = 0; a < sent->arg_num[h]; a++) {
		if (sent->args[h][a] == d && sent->arg_roles[h][a] == r) {
			return 1;
		}
	}
	return 0;
}


int check_pt_t_equality(PART_TREE *pt, SENTENCE *sent) {
	int w;
	for (w = 1; w <= sent->len; w++) {
		if (sent->head[w] != pt->nodes[w].head) {
			return 0;
		}
		if (sent->deprel[w] != pt->nodes[w].deprel) {
			return 0;
		}
	}
	return 1;
}
int check_ps_t_equality(PART_SRL *ps, SENTENCE *sent) {
	int w;
	for (w = 1; w <= sent->len; w++) {
		if (sent->sense[w] != ps->nodes[w].sense) {
			fprintf(stderr, "mismatched predicate, word %d:%s\n",
					w, sent->s_word[w]);
			return 0;
		}
		int i;
		for (i = 0; i < sent->arg_num[w]; i++) {
			int d = sent->args[w][i];
			int role = sent->arg_roles[w][i];
			if (! is_ps_role(ps, w, d, role)) {
				fprintf(stderr, "mismatched ps role, word %d:%s, role %d: %s\n",
						w, sent->s_word[w], i, sent->s_arg_roles[w][i]);
				return 0;
			}
		}
		for (i = 0; i < ps->nodes[w].arg_num; i++) {
			int d = ps->nodes[w].args[i];
			int role = ps->nodes[w].arg_roles[i];
			if (! is_srl_role(sent, w, d, role)) {
				fprintf(stderr, "mismatched srl role, word %d:%s, role %d: %d\n",
						w, sent->s_word[w], i, role);
				return 0;
			}
		}
	}
	return 1;
}

//check if it is possible to shift an element (if its left part is built)
int left_part_complete(PART_TREE *pt, SENTENCE *sent, int q) {
	//head is not found
	if (pt_head(pt, q) != sent->head[q] && sent->head[q] < q) {
		return 0;
	}

	int w;
	for (w = 1; w < q; w++) {
		if (sent->head[w] == q) {
			//check if it is already in the list of its children
			if (pt->nodes[w].head != q) {
				return 0;
			}
		}
	}
	return 1;
}
int srl_is_head_of(SENTENCE *sent, int h,  int d) {
	int a;
	for (a = 0; a < sent->arg_num[h]; a++) {
		if (sent->args[h][a] == d) {
			return 1;
		}
	}
	return 0;
}

int is_head_of(SENTENCE *sent, int h, int d) {
	return (sent->head[d] == h);
}



int srl_left_part_complete(PART_SRL *ps, SENTENCE *sent, int q) {
	//not all earlier heads are found
	int w;
	for (w = 1; w < q; w++) {
		if (srl_is_head_of(sent, w, q) != ps_is_head_of(ps, w, q)){
			return 0;
		}
		if (srl_is_head_of(sent, q, w) != ps_is_head_of(ps, q, w)) {
			return 0;
		}
	}
	return 1;
}


int everything_complete(PART_TREE *pt, SENTENCE *sent, int s) {
	//head is not found
	if (pt_head(pt, s) != sent->head[s]) {//&& sent->head[s] < s) {
		return 0;
	}

	int w;
	for (w = 1; w <= sent->len; w++) {
		if (sent->head[w] == s) {
			//check if it is already in the list of its children
			if (pt->nodes[w].head != s) {
				return 0;
			}
		}
	}
	return 1;
}
int srl_everything_complete(PART_SRL *ps, SENTENCE *sent, int q) {
	//not all earlier heads are found
	int w;
	for (w = 1; w <= sent->len; w++) {
		if (srl_is_head_of(sent, w, q) != ps_is_head_of(ps, w, q)){
			return 0;
		}
		if (srl_is_head_of(sent, q, w) != ps_is_head_of(ps, q, w)) {
			return 0;
		}
	}
	return 1;
}


int get_word_by_spec_hh(OPTION *opt, HH_LINK_SPEC *spec) {
	PART_TREE *pt = opt->pt;
	ASSERT(opt->pt != NULL);
	int w = -1;

	if (spec->orig_step_type != ANY_STEP && spec->orig_step_type != opt->step_type) {
		return 0;
	}


	ASSERT(spec->orig_offset >= 0);
	if (spec->orig_src == INP_SRC) {
		w = opt->queue + spec->orig_offset;
	} else if (spec->orig_src == STACK_SRC) {
		w = st_look(&opt->stack, spec->orig_offset);
	} else if (spec->orig_src == SRL_STACK_SRC) {
		w = st_look(&opt->srl_stack, spec->orig_offset);
	} else {
		ASSERT(0);
	}
	if (w <= 0 || w > pt->len) {
		return 0;
	}

	if (spec->orig_arg_bank >= 0) {
		//child
		w = ps_arg(opt->ps, w, spec->orig_arg_role);
		if (w == 0) {
			return 0;
		}
	}


	int h;
	ASSERT(spec->orig_head >= 0);
	for (h = 0; h < spec->orig_head; h++) {
		w = pt_head(pt, w);

		if (w == 0) {
			return 0;
		}
	}

	if (spec->orig_rrc > 0) {
		int c;
		for (c = 0; c < spec->orig_rrc; c++) {
			w = pt_rrc(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	} else {
		int c;
		for (c = 0; c < - spec->orig_rrc; c++) {
			w = pt_llc(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	}

	if (spec->orig_rs > 0) {
		int c;
		for (c = 0; c < spec->orig_rs; c++) {
			w = pt_rs(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	} else {
		int c;
		for (c = 0; c < - spec->orig_rs; c++) {
			w = pt_ls(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	}

	return w;
}


//return word
// 0 if feature is not available (e.g. no such word)
// -1 if feature is irrelevant for the current step
int get_word_by_route(OPTION *opt, NODE_ROUTE *spec, int spec_step_type) {
	PART_TREE *pt = opt->pt;
	ASSERT(opt->pt != NULL);
	int w = -1;

	if (spec_step_type != ANY_STEP && spec_step_type != opt->step_type) {
		return -1;
	}

	ASSERT(spec->offset_curr >= 0);
	if (spec->src == INP_SRC) {
		w = opt->queue + spec->offset_curr;
	} else if (spec->src == STACK_SRC) {
		w = st_look(&opt->stack, spec->offset_curr);
	} else if (spec->src == SRL_STACK_SRC) {
		w = st_look(&opt->srl_stack, spec->offset_curr);
	} else {
		ASSERT(0);
	}
	if (w <= 0 || w > pt->len) {
		return 0;
	}

	w += spec->offset_orig;

	if (w <= 0 || w > pt->len) {
		return 0;
	}

	if (spec->arg_bank >= 0) {
		//child
		w = ps_arg(opt->ps, w, spec->arg_role);
		if (w == 0) {
			return 0;
		}
	}

	int h;
	ASSERT(spec->head >= 0);
	for (h = 0; h < spec->head; h++) {
		w = pt_head(pt, w);

		if (w == 0) {
			return 0;
		}
	}

	if (spec->rc > 0) {
		int c;
		for (c = 0; c < spec->rc; c++) {
			w = pt_rc(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	} else {
		int c;
		for (c = 0; c < - spec->rc; c++) {
			w = pt_lc(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	}

	if (spec->rs > 0) {
		int c;
		for (c = 0; c < spec->rs; c++) {
			w = pt_rs(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	} else {
		int c;
		for (c = 0; c < - spec->rs; c++) {
			w = pt_ls(pt, w);
			if (w == 0) {
				return 0;
			}
		}
	}

	return w;
}

int get_prev_deprel(MODEL_PARAMS *mp, OPTION *opt) {
	ASSERT(opt->new_state);
	int deprel;
	if (opt->previous_act == LA || opt->previous_act == RA) {
		deprel = opt->previous_output;
		ASSERT(deprel >= 0 && deprel < mp->deprel_num);
	} else {
		deprel = 0;
	}
	return deprel;
}

int get_prev_arg_role(MODEL_PARAMS *mp, OPTION *opt) {
	ASSERT(opt->new_state);
	int b;
	for (b = 0; b <BANK_NUM; b++) {
		if (opt->previous_act == SRL_LA[b] || opt->previous_act == SRL_RA[b]) {
			int role = opt->previous_output;
			ASSERT(role >= 0 && role < mp->role_num[b]);
			return role;
		}
	}
	return 0;
}

void set_ih_links(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *sent) {
	ASSERT(opt->inp_num == 0);
	PART_TREE *pt = opt->pt;



	if (opt->new_state) {
		//Debug
		/*        printf("IH_LINKS: "); */
		//create links using specifications
		if (opt->previous_option == NULL) {
			opt->inp_biases[opt->inp_num++] = &mp->init_bias;
			//Debug
			/*            printf("<init>\n"); */
			return;
		} else {
			if (ap->disting_biases) {
				if (opt->step_type == SYNT_STEP) {
					opt->inp_biases[opt->inp_num++] = &mp->hid_bias;
				} else {
					ASSERT(opt->step_type == SRL_STEP);
					opt->inp_biases[opt->inp_num++] = &mp->hid_bias_srl;
				}
			} else {
				opt->inp_biases[opt->inp_num++] = &mp->hid_bias;
			}

			ASSERT(opt->previous_act >= 0 && opt->previous_act < ACTION_NUM);
			opt->inp_biases[opt->inp_num++] = &mp->prev_act_bias[opt->previous_act];
			int deprel = get_prev_deprel(mp, opt);
			opt->inp_biases[opt->inp_num++] = &mp->prev_deprel_bias[opt->previous_act][deprel];
			int arg_role = get_prev_arg_role(mp, opt);
			opt->inp_biases[opt->inp_num++] = &mp->prev_arg_bias[opt->previous_act][arg_role];


			//Debug
			/*            printf(" /prev_act = %d, prev_depr = %d/", opt->previous_act, deprel); */

		}


		int l;
		for (l = 0; l < mp->ih_links_num; l++) {
			if (opt->inp_num >= MAX_IH_LINKS_OPT) {
				fprintf(stderr, "Too many features in an option, exceeded MAX_IH_LINKS_OPT = %d\n", MAX_IH_LINKS_OPT);
				exit(1);
			}
			IH_LINK_SPEC *spec = &mp->ih_links_specs[l];
			int w = get_word_by_route(opt, &spec->node, spec->step_type);
			//this feature is irrelevant
			if (w < 0) {
				continue;
			}
			//Debug
			/*            printf(" %d(%d)", l, w); */

			if (w > opt->period - 1 + ap->input_offset) {
				fprintf(stderr, "Warning: IH link %d defines feature of the right context word (queue = %d, word = %d)\n",
						l, opt->period, w);
			}
			if (spec->feature_class == NODE_FEATURE_CLASS) {
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
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_deprel[l][deprel];
				} else if (spec->info_type == POS_TYPE) {
					int pos;
					if (w > 0) {
						pos = sent->pos[w];
					} else {
						pos = mp->pos_info_in.num;
					}
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_pos[l][pos];
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
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_word[l][pos][feat][word];
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
						opt->inp_biases[opt->inp_num++] = &mp->ih_link_feat[l][pos][feat];
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
								opt->inp_biases[opt->inp_num++] = &mp->ih_link_elfeat[l][elfeat];
							}
						} else {
							opt->inp_biases[opt->inp_num++] = &mp->ih_link_elfeat[l][mp->elfeat_num];
						}
					}

				} else if (spec->info_type == CPOS_TYPE) {
					int cpos;
					if (w > 0) {
						cpos = sent->cpos[w];
					} else {
						cpos = mp->cpos_num;
					}
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_cpos[l][cpos];
				} else if (spec->info_type == LEMMA_TYPE) {
					int lemma;
					if (w > 0) {
						lemma  = sent->lemma[w];
					}   else {
						lemma = mp->lemma_num;
					}
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_lemma[l][lemma];
				} else if (spec->info_type == SENSE_TYPE) {
					int bank, lemma = 0, sense = 0;
					if (w > 0) {
						bank = sent->bank[w];
						sense = opt->ps->nodes[w].sense;
						lemma = sent->lemma[w];
						if (bank < 0 || sense < 0) {
							//no sense for the word
							bank = BANK_NUM + 1;
							sense = 0;
							lemma = 0;
						}
					} else {
						//no word
						bank = BANK_NUM;
					}
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_sense[l][bank][lemma][sense];
				} else {
					ASSERT(0);
				}
			} else if (spec->feature_class == PATH_FEATURE_CLASS)  {
				if (spec->info_type == SYNT_LAB_PATH_TYPE) {
					int w_other = get_word_by_route(opt, &spec->node_other, spec->step_type);
					//this feature is irrelevant
					if (w_other < 0) {
						continue;
					}
					if (w_other > opt->period - 1 + ap->input_offset) {
						fprintf(stderr, "Warning: IH link %d defines feature of the right context word (queue = %d, word = %d)\n",
								l, opt->period, w_other);
					}
					int path_id = -1;
					if (w  == 0 || w_other == 0) {
						path_id = get_synt_path_feature_size(mp);
					} else {
						path_id = get_synt_path_feature_id(mp, opt, w, w_other);
					}
					opt->inp_biases[opt->inp_num++] = &mp->ih_link_synt_lab_path[l][path_id];
				} else {
					ASSERT(0);
				}

			} else {
				ASSERT(0);
			}
		}
		//Debug
		/*        printf("\n"); */
	} else {
		memcpy(opt->inp_biases, opt->previous_option->inp_biases, sizeof(IH_LINK*) * opt->previous_option->inp_num);
		opt->inp_num  = opt->previous_option->inp_num;
	}
}

//returns parser action made before create this opt (opt->new_state == true)
int act_before(OPTION *opt) {
	ASSERT(opt->previous_option != NULL);
	ASSERT(opt->new_state);
	OPTION *prev_opt = opt->previous_option, *after_opt = opt;
	while (!prev_opt->new_state && prev_opt != NULL) {
		after_opt = prev_opt;
		prev_opt = prev_opt->previous_option;
	}
	ASSERT(prev_opt != NULL);
	return after_opt->previous_output;
}

#define NEEDED_STEP_TYPE(spec, opt) ((spec)->target_step_type == ANY_STEP || (spec)->target_step_type == (opt)->step_type)

void set_hh_links(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *sent) {
	ASSERT(opt->rel_num  == 0);

	if (opt->new_state) {
		//create links using specifications
		int l;
		for (l = 0; l < mp->hh_links_num; l++) {
			HH_LINK_SPEC *spec = &mp->hh_links_specs[l];
			ASSERT(spec->orig_offset >= 0 && spec->target_offset >= 0);

			//find original w
			int orig_w = -1;
			orig_w = get_word_by_spec_hh(opt, spec);

			if (orig_w == 0) {
				continue;
			}

			LAYER *rel_layer = NULL;
			OPTION *prev_opt = opt->previous_option;
			//offset defines how many states we passed (previous state is 1)
			int offset = 0;
			OPTION *after_opt = opt;
			if (spec->target_src == INP_SRC) {
				while (prev_opt != NULL) {
					//passed new_state
					if (after_opt->new_state && NEEDED_STEP_TYPE(spec, after_opt)) {
						offset++;
					}
					if (NEEDED_STEP_TYPE(spec, prev_opt) &&  after_opt->new_state && prev_opt->queue + spec->target_offset == orig_w
							&& (spec->offset < 0 || spec->offset == offset)) {
						if (spec->act == ANY || spec->act == act_before(after_opt)) {
							rel_layer = &after_opt->prev_layer;
							if (spec->when == FIND_LAST || spec->offset >= 0) {
								break;
							}
						}
					}

					after_opt = prev_opt;
					prev_opt = prev_opt->previous_option;
				}
			} else if (spec->target_src == STACK_SRC) {
				while (prev_opt != NULL) {
					//passed new_state
					if (after_opt->new_state && NEEDED_STEP_TYPE(spec, after_opt)) {
						offset++;
					}
					if (NEEDED_STEP_TYPE(spec, prev_opt) && after_opt->new_state && st_look(&prev_opt->stack, spec->target_offset) == orig_w
							&& (spec->offset < 0 || spec->offset == offset)) {
						if (spec->act == ANY || spec->act == act_before(after_opt)) {
							rel_layer = &after_opt->prev_layer;
							if (spec->when == FIND_LAST || spec->offset >= 0) {
								break;
							}
						}
					}
					after_opt = prev_opt;
					prev_opt = prev_opt->previous_option;
				}
			} else if (spec->target_src == SRL_STACK_SRC) {
				while (prev_opt != NULL) {
					//passed new_state
					if (after_opt->new_state && NEEDED_STEP_TYPE(spec, after_opt)) {
						offset++;
					}
					if (NEEDED_STEP_TYPE(spec, prev_opt) && after_opt->new_state && st_look(&prev_opt->srl_stack, spec->target_offset) == orig_w
							&& (spec->offset < 0 || spec->offset == offset)) {
						if (spec->act == ANY || spec->act == act_before(after_opt)) {
							rel_layer = &after_opt->prev_layer;
							if (spec->when == FIND_LAST || spec->offset >= 0) {
								break;
							}
						}
					}
					after_opt = prev_opt;
					prev_opt = prev_opt->previous_option;
				}
			} else {
				ASSERT(0);
			}
			if (rel_layer != NULL) {
				opt->rel_layers[opt->rel_num] = rel_layer;
				opt->rel_weights[opt->rel_num] = &mp->hh_link[l];
				opt->rel_num++;
			}


		} // l
	} else {
		//copy links from the previous
		memcpy(opt->rel_weights, opt->previous_option->rel_weights, sizeof(HH_LINK*) * opt->previous_option->rel_num);
		memcpy(opt->rel_layers, opt->previous_option->rel_layers, sizeof(LAYER*) * opt->previous_option->rel_num);
		opt->rel_num = opt->previous_option->rel_num;
	}
}

void set_links(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *sent) {
	set_ih_links(ap, mp, opt, sent);
	set_hh_links(ap, mp, opt, sent);
}
#define TRY_FREE_PARTIALS(pt, ps) if (do_free_partials) {  pt_free(pt); ps_free(ps); }

OPTION * create_word_prediction_seq(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, OPTION *opt, int do_free_partials) {
	ASSERT(ap->input_offset == 1 || ap->input_offset == 0);
	int pos = sent->pos[opt->period + ap->input_offset];
	int feat = sent->feat_out[opt->period + ap->input_offset];
	int word = sent->word_out[opt->period + ap->input_offset];


	OPTION *new_opt = create_option(SYNT_STEP, 0, opt->previous_act, pos, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, NULL, NULL, &mp->out_link_feat[pos], mp->pos_info_out.feat_infos[pos].num);
	set_links(ap, mp, new_opt, sent);


	new_opt = create_option(SYNT_STEP, 0, opt->previous_act, feat, new_opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, NULL, NULL,  &mp->out_link_word[pos][feat],
			mp->pos_info_out.feat_infos[pos].word_infos[feat].num);
	set_links(ap, mp, new_opt, sent);

	new_opt = create_option(SYNT_STEP, 1, SHIFT, word, new_opt, opt->period + 1, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);

	TRY_FREE_PARTIALS(&(opt->pt), &(opt->ps));

	st_push(&new_opt->stack, new_opt->queue);
	st_push(&new_opt->srl_stack, new_opt->queue);
	new_opt->queue++;
	set_links(ap, mp, new_opt, sent);

	return new_opt;
}



int does_restrict(ACTION_SET *as) {
	int a;
	for (a = 0; a < ACTION_NUM; a++) {
		if (!as->acts[a]) {
			return 1;
		}

	}
	return 0;
}


void set_mask(MODEL_PARAMS *mp, OPTION *opt, ACTION_SET *as) {
	ASSERT(opt->outputs.num == ACTION_NUM && opt->out_link == &mp->out_link_act);

	//is it actially masked
	if (does_restrict(as)) {
		if (opt->outputs.mask == NULL)
			alloc_option_outputs(opt);
		opt->outputs.is_masked = 1;
		int a;
		for (a = 0; a < ACTION_NUM; a++) {
			opt->outputs.mask[a] = as->acts[a];
		}
	} else {
		//do nothing, everything is set to 'allowed' in the ACTION_SET as
	}
}

OPTION* _action_srl_red(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, int do_free_partials, OPTION *opt) {
	//create sequence of reduce operation
	//update queue and stack
	opt = create_option(SRL_STEP, 1, SRL_RED, SRL_RED, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
			pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
	TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));

	st_pop(&opt->srl_stack);
	set_links(ap, mp, opt, sent);
	return opt;
}

OPTION* _action_srl_flip(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent,  int do_free_partials, OPTION *opt) {
	//create sequence of reduce operation
	//update queue and stack
	opt = create_option(SRL_STEP, 1, SRL_FLIP, SRL_FLIP, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
			pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
	TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));


	int srl_s = st_pop(&opt->srl_stack);
	int srl_s_under = st_pop(&opt->srl_stack);

	st_push(&opt->srl_stack, srl_s);
	st_push(&opt->srl_stack, srl_s_under);

	set_links(ap, mp, opt, sent);
	return opt;
}

OPTION* _action_synt_flip(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, int do_free_partials, OPTION *opt) {
	opt = create_option(SYNT_STEP, 1,  FLIP, FLIP, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
			pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
	TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));

	int s = st_pop(&opt->stack);
	int s_under = st_pop(&opt->stack);
	st_push(&opt->stack, s);
	st_push(&opt->stack, s_under);

	set_links(ap, mp, opt, sent);

	return opt;
}

OPTION* _action_synt_red(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, int do_free_partials, OPTION *opt) {
	//create sequence of reduce operation
	//update queue and stack
	opt = create_option(SYNT_STEP, 1, RED, RED, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
			pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
	TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));

	st_pop(&opt->stack);
	set_links(ap, mp, opt, sent);
	return opt;
}
int get_useful(STACK *stack, int idx, int *right_connections_num,  int *right_degree) {
	int w;
	while ((w = st_look(stack, idx++)) > 0) {
		if (right_degree[w] != right_connections_num[w]) {
			return w;
		}
	}
	return -1;
}


//returns how many POS outputs are needed - takes in account $$$ and ^^^
#define get_pos_out_num() ((ap->input_offset > 0) ? mp->pos_info_out.num + ap->input_offset : mp->pos_info_out.num)

//processes sentence and creates derivations
//returns tail of the derivation
OPTION *get_derivation(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, int do_free_partials) {

	/*    if (ap->intern_synt_deproj == DEPROJ_EXHAUSTIVE && !ap->is_synt_early_reduce) {
        fprintf(stderr, "Error: exhaustive internal syntactic projectivization with late reduce strategy is not supported\n");
        exit(EXIT_FAILURE);
    }
    if (ap->intern_srl_deproj == DEPROJ_EXHAUSTIVE && !ap->is_srl_early_reduce) {
        fprintf(stderr, "Error: exhaustive internal SRL projectivization with late reduce strategy is not supported\n");
        exit(EXIT_FAILURE);
    } */

	//Debug
	//    printf("Next sentence ============\n");

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);

	STACK st; st.size = 0;
	STACK srl_st; srl_st.size = 0;
	//OPTION *head = create_option(SYNT_STEP, 1, -1, -1, NULL, 1, &st, &srl_st, 1, pt_alloc(sent->len), ps_alloc(sent->len), &mp->out_link_act, ACTION_NUM);
	OPTION *head = create_option(SRL_STEP, 1, -1, -1, NULL, 1, &st, &srl_st, 1, pt_alloc(sent->len), ps_alloc(sent->len), &mp->out_link_act, ACTION_NUM);
	///st_push(&head->srl_stack, 0); //JH allow 0 as an SRL argument
	set_links(ap, mp, head, sent);

	///printf("new sentence\n");///
	OPTION *opt = head;
	while (opt->queue != sent->len + 1) {
		//get next possible actions
		ACTION_SET as = get_next_actions(ap, mp, opt, sent);

		set_mask(mp, opt, &as);

		//=====================  SYNTAX ======================================================
		if (ap->is_synt_early_reduce && as.acts[RED]) {
			int s = st_peek(&opt->stack);
			if (everything_complete(opt->pt, sent, s)) {
				if (ap->parsing_mode != 3 || sent->head[s] != 0 || sent->deprel[s] != ROOT_DEPREL) {
					opt = _action_synt_red(ap, mp, sent, do_free_partials, opt);
					//DEBUG
					//printf("RED\n");

					continue;
				}
			}
		}


		//check LA
		if (as.acts[LA]) {
			int d = st_peek(&opt->stack);
			int h = opt->queue;
			if (sent->head[d] == h) {
				//create sequence of LA operations
				//update queue and stack
				opt = create_option(SYNT_STEP, 0, opt->previous_act, LA, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
						&mp->out_link_la_label, mp->deprel_num);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
				//opt->previous_option->pt = NULL;
				set_links(ap, mp, opt, sent);

				opt = create_option(SYNT_STEP, 1, LA, sent->deprel[d], opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
						&mp->out_link_act, ACTION_NUM);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
				//opt->previous_option->pt = NULL;
				pt_add_link(opt->pt, h, d, sent->deprel[d]);
				opt->pt->right_connections_num[d]++;
				if (ap->intern_synt_deproj == DEPROJ_NO) {
					st_pop(&opt->stack);
				} else {
					// Do Nothing!!!
				}
				set_links(ap, mp, opt, sent);


				//DEBUG
				//printf("LA\n");

				continue;
			}
		}

		//check RA
		if (as.acts[RA]) {
			int d = opt->queue;
			int h = st_peek(&opt->stack);
			//when agrees
			if (sent->head[d] == h) {
				//means if stack is not empty or  we can make RA with empty stack
				if (h != 0 ||
						(ap->parsing_mode >= 3 && sent->deprel[d] != ROOT_DEPREL
								// can do only once
								&& opt->pt->nodes[d].deprel == ROOT_DEPREL))
				{
					//create sequence of RA operations
					//update queue and stack
					opt = create_option(SYNT_STEP, 0, opt->previous_act, RA, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt),  ps_clone(opt->ps),
							&mp->out_link_ra_label, mp->deprel_num);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					set_links(ap, mp, opt, sent);


					opt = create_option(SYNT_STEP, 1, RA, sent->deprel[d], opt, opt->period, &opt->stack,  &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
							&mp->out_link_act, ACTION_NUM);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					pt_add_link(opt->pt, h, d, sent->deprel[d]);
					if (h != 0) {
						opt->pt->right_connections_num[h]++;
					}
					set_links(ap, mp, opt, sent);

					//DEBUG
					//printf("RA\n");

					continue;
				}
			}
		}
		// check FLIP
		if (as.acts[FLIP] && ap->intern_synt_deproj == DEPROJ_EXHAUSTIVE) {
			int s = st_peek(&opt->stack);
			int s_already_conns =  opt->pt->right_connections_num[s];
			int s_under = st_look(&opt->stack, 1);
			int s_under_already_conns = opt->pt->right_connections_num[s_under];

			int top_is_usl = (sent->synt_right_degree[s] == s_already_conns);
			int under_top_is_usl = (sent->synt_right_degree[s_under] == s_under_already_conns);

			if (ap->is_synt_early_reduce || (!top_is_usl && !under_top_is_usl)  )  {
				// s_under will be active earlier than s
				if (lexicograph_ignore_double_entries_comp(
						sent->synt_right_indices[s_under] + s_under_already_conns,
						sent->synt_right_degree[s_under] - s_under_already_conns,
						sent->synt_right_indices[s] + s_already_conns,
						sent->synt_right_degree[s] - s_already_conns) < 0) {
					//create sequence of reduce operation
					//update queue and stack
					opt = _action_synt_flip(ap, mp, sent, do_free_partials, opt);
					//DEBUG
					printf("(SYNT) FLIP\n");
					continue;
				}

			} else {
				// if a top word can be reduce from the top
				if (top_is_usl) {
					int first_usf = get_useful(&opt->stack, 1, opt->pt->right_connections_num, sent->synt_right_degree);
					if (first_usf > 0) {
						// first 'useful' word in the stack should be attached to front of queue -- start remioving useless words
						// next words will be removed on the next rounds
						int next_connection = sent->synt_right_indices[first_usf][opt->pt->right_connections_num[first_usf]];
						if (next_connection  == opt->queue) {
							ASSERT(as.acts[RED]);
							opt = _action_synt_red(ap, mp, sent, do_free_partials, opt);
							//DEBUG
							//printf("RED\n");
							continue;
						}
					}
				}  else {
					// if
					int second_usf = get_useful(&opt->stack, 1, opt->pt->right_connections_num, sent->synt_right_degree);

					ASSERT(second_usf != s_under);
					if (second_usf > 0) {
						int second_usf_already_conns = opt->pt->right_connections_num[second_usf];

						if (lexicograph_ignore_double_entries_comp(
								sent->synt_right_indices[second_usf] + second_usf_already_conns,
								sent->synt_right_degree[second_usf] - second_usf_already_conns,
								sent->synt_right_indices[s] + s_already_conns,
								sent->synt_right_degree[s] - s_already_conns) < 0) {

							opt = _action_synt_flip(ap, mp, sent, do_free_partials, opt);
							as = get_next_actions(ap, mp, opt, sent);
							set_mask(mp, opt, &as);
							ASSERT(as.acts[RED]);
							opt = _action_synt_red(ap, mp, sent, do_free_partials, opt);
							// DEBUG
							printf("FLIP, followed by RED\n");
							//printf("RED, preceded by FLIP\n");
							continue;
						}
					}

				}   // !top_is_usl
			} // !ap->is_synt_early_reduce
		}  // FLIP


		//check SWITCH
		if (as.acts[SWITCH]) {
			int q = opt->queue;
			if (left_part_complete(opt->pt, sent, q)) {
				//if atached to root with not ROOT_DEPREL and parsing_mode >= 3 then RA, not SHIFT should be peformed
				//option sent->deprel[q] == opt->pt->nodes[q]->deprel - relates to RA- + SHIFT sequence  and means that RA- is actually preformed
				//and q is already attached
				if (sent->head[q] != 0 || ap->parsing_mode < 3 ||
						(ap->parsing_mode >= 3 && (sent->deprel[q] == ROOT_DEPREL || sent->deprel[q] == opt->pt->nodes[q].deprel))) {

					opt = create_option(SRL_STEP, 1, SWITCH, SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
							pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
					/*                    opt = create_option(SYNT_STEP, 1, SWITCH, SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
                            pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM); */
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					set_links(ap, mp, opt, sent);

					//DEBUG
					//printf("SWITCH\n");
					continue;
				}
			}
		}

		//check RED
		if (!ap->is_synt_early_reduce && as.acts[RED]) {
			int s = st_peek(&opt->stack);
			if (everything_complete(opt->pt, sent, s)) {
				if (ap->parsing_mode != 3 || sent->head[s] != 0 || sent->deprel[s] != ROOT_DEPREL) {

					opt = _action_synt_red(ap, mp, sent, do_free_partials, opt);

					//DEBUG
					//printf("RED\n");

					continue;
				}
			}
		}

		// check FLIP
		if (as.acts[FLIP] && ap->intern_synt_deproj != DEPROJ_EXHAUSTIVE) {
			opt = _action_synt_flip(ap, mp, sent, do_free_partials, opt);

			//DEBUG
			printf("(SYNT) FLIP\n");

			continue;
		}

		//=====================  WORD PREDICTION =============================================
		if (as.acts[SHIFT]) {
			//create sequence of shift operation
			//update queue and stack
			opt = create_option(SYNT_STEP, 0, opt->previous_act, SHIFT, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
					&mp->out_link_pos,  get_pos_out_num());
			TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
			set_links(ap, mp, opt, sent);

			opt = create_word_prediction_seq(ap, mp, sent, opt, do_free_partials);

			//DEBUG
			//printf("SHIFT\n");
			continue;
		}




		//=====================  SRL  ======================================================

		if (ap->is_srl_early_reduce && as.acts[SRL_RED]) {
			int srl_s = st_peek(&opt->srl_stack);
			if (srl_everything_complete(opt->ps, sent, srl_s)) {

				opt = _action_srl_red(ap, mp, sent, do_free_partials, opt);

				//DEBUG
				//                    printf("SRL_RED\n");

				continue;
			}
		}

		//check SRL_LA
		int q_bank = sent->bank[opt->queue];
		///if (q_bank >= 0)///
		///  printf("srl_la 0: %d %d; %d %d\n", q_bank, as.acts[SRL_LA[q_bank]], st_peek(&opt->srl_stack), opt->queue);///
		if (q_bank >= 0 && as.acts[SRL_LA[q_bank]]) {
			int d = st_peek(&opt->srl_stack);
			int h = opt->queue;
			int role = next_srl_role(0, sent, h, d);
			///printf("srl_la 1: %d %d %d\n", d, h, role);///
			if (role >= 0) {
				int i = 0;
				for (; role >= 0; role = next_srl_role(i, sent, h, d)) {
					i++;
					//create sequence of SRL_LA operations
					//update queue and stack
					opt = create_option(SRL_STEP, 0, opt->previous_act, SRL_LA[q_bank], opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
							&mp->out_link_sem_la_label[q_bank], mp->role_num[q_bank]);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					set_links(ap, mp, opt, sent);

					opt = create_option(SRL_STEP, 1, SRL_LA[q_bank], role, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
							&mp->out_link_act, ACTION_NUM);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					ps_add_link(opt->ps, h, d, role);
					opt->ps->right_connections_num[d]++;
					set_links(ap, mp, opt, sent);

					//DEBUG
					//printf("SRL_LA[%d]\n", q_bank);
				}

				continue;
			}
		}
		q_bank = -1;

		//check SRL_RA
		int s_bank = sent->bank[st_peek(&opt->srl_stack)];
		if (s_bank >= 0 && as.acts[SRL_RA[s_bank]]) {
			int d = opt->queue;
			int h = st_peek(&opt->srl_stack);
			int role =  next_srl_role(0, sent, h, d);
			///printf("srl_ra: %d %d %d\n", d, h, role);///
			//when agrees
			if (role >= 0) {
				int i = 0;
				for (; role >= 0; role = next_srl_role(i, sent, h, d)) {
					ASSERT(i == 0);
					i++;
					//create sequence of RA operations
					//update queue and stack
					opt = create_option(SRL_STEP, 0, opt->previous_act, SRL_RA[s_bank], opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt),  ps_clone(opt->ps),
							&mp->out_link_sem_ra_label[s_bank], mp->role_num[s_bank]);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					set_links(ap, mp, opt, sent);


					opt = create_option(SRL_STEP, 1, SRL_RA[s_bank], role, opt, opt->period, &opt->stack,  &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
							&mp->out_link_act, ACTION_NUM);
					TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
					ps_add_link(opt->ps, h, d, role);
					opt->ps->right_connections_num[h]++;
					set_links(ap, mp, opt, sent);

					//DEBUG
					//printf("SRL-RA[%d]\n", s_bank);
				}  // role

				continue;
			}
		}

		if (as.acts[SRL_FLIP] && ap->intern_srl_deproj == DEPROJ_EXHAUSTIVE) {
			int srl_s = st_peek(&opt->srl_stack);
			int srl_s_already_conns = opt->ps->right_connections_num[srl_s];

			int srl_s_under = st_look(&opt->srl_stack, 1);
			int srl_s_under_already_conns = opt->ps->right_connections_num[srl_s_under];


			int top_is_usl = (sent->srl_right_degree[srl_s] == srl_s_already_conns);
			int under_top_is_usl = (sent->srl_right_degree[srl_s_under] == srl_s_under_already_conns);

			if (ap->is_srl_early_reduce || (!top_is_usl && !under_top_is_usl)  )  {
				// srl_s_under will be active earlier than srl_s
				if (lexicograph_ignore_double_entries_comp(
						sent->srl_right_indices[srl_s_under] + srl_s_under_already_conns,
						sent->srl_right_degree[srl_s_under] - srl_s_under_already_conns,
						sent->srl_right_indices[srl_s] + srl_s_already_conns,
						sent->srl_right_degree[srl_s] - srl_s_already_conns) < 0) {

					opt = _action_srl_flip(ap, mp, sent, do_free_partials, opt);

					//DEBUG
					//TMP printf("SRL_FLIP\n");
					continue;
				}
			} else {
				// if a top word can be reduce from the top
				if (top_is_usl) {
					int first_usf = get_useful(&opt->srl_stack, 1, opt->ps->right_connections_num, sent->srl_right_degree);
					if (first_usf > 0) {
						// first 'useful' word in the stack should be attached to front of queue -- start remioving useless words
						// next words will be removed on the next rounds
						int next_connection = sent->srl_right_indices[first_usf][opt->ps->right_connections_num[first_usf]];
						if (next_connection  == opt->queue) {
							ASSERT(as.acts[SRL_RED]);
							opt = _action_srl_red(ap, mp, sent, do_free_partials, opt);
							continue;
							//DEBUG
							//printf("SRL_RED, useless reduction\n");
						}
					}
				}  else {
					// if
					int second_usf = get_useful(&opt->srl_stack, 1, opt->ps->right_connections_num, sent->srl_right_degree);

					ASSERT(second_usf != srl_s_under);

					if (second_usf > 0) {
						int second_usf_already_conns = opt->ps->right_connections_num[second_usf];

						if (lexicograph_ignore_double_entries_comp(
								sent->srl_right_indices[second_usf] + second_usf_already_conns,
								sent->srl_right_degree[second_usf] - second_usf_already_conns,
								sent->srl_right_indices[srl_s] + srl_s_already_conns,
								sent->srl_right_degree[srl_s] - srl_s_already_conns) < 0) {

							opt = _action_srl_flip(ap, mp, sent, do_free_partials, opt);
							as = get_next_actions(ap, mp, opt, sent);
							set_mask(mp, opt, &as);
							ASSERT(as.acts[SRL_RED]);
							opt = _action_srl_red(ap, mp, sent, do_free_partials, opt);
							// DEBUG
							printf("SRL_FLIP, followed by SRL_RED\n");
							//printf("SRL_RED, preceded by SRL_FLIP\n");

							continue;
						}
					}
				}   // !top_is_usl
			} // !ap->is_synt_early_reduce
		}  // FLIP

		//check SRL_SWITCH
		if (as.acts[SRL_SWITCH]) {
			int q = opt->queue;
			if (srl_left_part_complete(opt->ps, sent, q)) {

				opt = create_option(SYNT_STEP, 1, SRL_SWITCH, SRL_SWITCH, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
						pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
				set_links(ap, mp, opt, sent);

				//DEBUG
				//printf("SRL_SWITCH\n");
				continue;
			}
		}

		//check SRL_RED
		if (!ap->is_srl_early_reduce  && as.acts[SRL_RED]) {
			int srl_s = st_peek(&opt->srl_stack);
			if (srl_everything_complete(opt->ps, sent, srl_s)) {
				opt = _action_srl_red(ap, mp, sent, do_free_partials, opt);
				//DEBUG
				//printf("SRL_RED\n");

				continue;
			}
		}

		if (as.acts[PREDIC_NO]) {
			int q = opt->queue;
			if (sent->sense[q] < 0) {
				opt = create_option(SRL_STEP, 1, PREDIC_NO, PREDIC_NO, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue,
						pt_clone(opt->pt), ps_clone(opt->ps), &mp->out_link_act, ACTION_NUM);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));

				set_links(ap, mp, opt, sent);

				//DEBUG
				//printf("PREDIC_NO\n");
				continue;
			}
		}
		if (as.acts[PREDIC_YES]) {
			int q = opt->queue;
			int q_sense = sent->sense[q];
			if (q_sense >= 0) {

				int q_bank = sent->bank[q];
				int q_lemma = sent->lemma[q];

				//create sequence of predicate prediction operations
				opt = create_option(SRL_STEP, 0, opt->previous_act, PREDIC_YES, opt, opt->period, &opt->stack, &opt->srl_stack, opt->queue, pt_clone(opt->pt),  ps_clone(opt->ps),
						&mp->out_link_sense[q_bank][q_lemma][0], mp->sense_num[q_bank][q_lemma]);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
				set_links(ap, mp, opt, sent);


				opt = create_option(SRL_STEP, 1, PREDIC_YES, q_sense, opt, opt->period, &opt->stack,  &opt->srl_stack, opt->queue, pt_clone(opt->pt), ps_clone(opt->ps),
						&mp->out_link_act, ACTION_NUM);
				TRY_FREE_PARTIALS(&(opt->previous_option->pt), &(opt->previous_option->ps));
				ps_set_sense(opt->ps, q, q_sense);
				set_links(ap, mp, opt, sent);

				//DEBUG
				//printf("PREDIC_YES\n");
				continue;
			}
		}
		if (as.acts[SRL_FLIP] && ap->intern_srl_deproj != DEPROJ_EXHAUSTIVE) {
			opt = _action_srl_flip(ap, mp, sent, do_free_partials, opt);
			//DEBUG
			//printf("SRL_FLIP\n");

			continue;
		}

		// if stalled in syntactic part
		if (IS_NOW_SYNTAX(opt->previous_act)) {
			//DEBUG
			printf("STALLED IN SYNTAX\n");

		} else {
			//DEBUG
			printf("STALLED IN SRL\n");
		}

		ASSERT(0);
		if (IS_NOW_SYNTAX(opt->previous_act)) {
			fprintf(stderr, "(Syntax) Problem: wrong parsing order or in input file (e.g. NON-PROJECTIVITY): can't make an action\n");
			printf("(Syntax) Problem: wrong parsing order or in input file (e.g. NON-PROJECTIVITY): can't make an action\n");
		} else {
			//TMP fprintf(stderr, "(SRL) Problem: wrong parsing order or in input file (e.g. NON-PROJECTIVITY): can't make an action\n");
			//TMP printf("(SRL) Problem: wrong parsing order or in input file (e.g. NON-PROJECTIVITY): can't make an action\n");
		}
		fprintf(stderr, "Try changing PARSING_ORDER in the configuration file to a larger value\n");
		//        printf("%s\n", print_sent(sent, 1));
		//TODO this should be a fatal error
		fprintf(stderr, "Warning: returning partial derivation\n");
		DEF_ALLOC(as_final, ACTION_SET);
		bzero(as_final, sizeof(ACTION_SET));
		set_mask(mp, opt, as_final);
		free(as_final);

		TRY_FREE_PARTIALS(&(opt->pt), &(opt->ps));
		return opt;
		//TODO restore!
		//exit(1);
	}

	//DEBUG
	//TMP printf("==== SENT FINISHED ===== \n");

	if (!check_pt_t_equality(opt->pt, sent)) {
		fprintf(stderr, "Presumably: bug in parsing or in input file (e.g. NON-PROJECTIVITY): resulting syntactic trees do not match\n");
		exit(1);
	}
	if (!check_ps_t_equality(opt->ps, sent)) {
		fprintf(stderr, "Presumably: bug in SRL parsing or in input file (e.g. NON-PROJECTIVITY): resulting predicate argument structures do not match\n");

	}


	DEF_ALLOC(as, ACTION_SET);
	bzero(as, sizeof(ACTION_SET));
	set_mask(mp, opt, as);
	free(as);

	TRY_FREE_PARTIALS(&(opt->pt), &(opt->ps));



	return opt;
}


//seq is assumed to include both words and tags
OPTION* get_derivation_list (APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent,
		OPTION_LIST **der_first, OPTION_LIST **der_last, int do_free_partials) {

	*der_last = increase_list(NULL, NULL);
	*der_first = *der_last;
	OPTION *last_opt = get_derivation(ap, mp, sent, do_free_partials);

	OPTION *opt = last_opt;

	while (opt->previous_option != NULL) {
		opt = opt->previous_option;
	}

	while (1) {
		*der_last = increase_list(*der_last, opt);
		if (opt->next_options->prev == NULL) {
			break;
		}
		opt = opt->next_options->prev->elem;
	}

	return last_opt;
}

//actually this is not path number but rather a convenient encoding size (though with gaps)
int  get_synt_path_feature_size(MODEL_PARAMS *mp) {

	int path_num  = 1;
	int l;
	for (l = 1; l <= MAX_LAB_PATH_LEN; l++) {
		// +1 to allow variable length paths
		path_num *= 2 * mp->deprel_num + 1;
	}
	// 0 path is also path (the same node/or not the same node test is possible)
	return path_num + 2; //???
}

/** computes path id for directed syntactic labels */
int get_synt_path_feature_id(MODEL_PARAMS *mp, OPTION *opt, int w, int w_other) {
	int dist = pt_get_nondirected_path(opt->pt, w, w_other, MAX_LAB_PATH_LEN);
	if (dist < 0) {
		// no path
		return 0;
	}
	PART_TREE *pt = opt->pt;

	// 1 is to signify: w = w_other  (0 path)
	int path_id = 1;
	int l = pt->nodes[w_other]._distance;
	int next_node = w_other;
	int d = 1;
	int s = 0;
	while (pt->nodes[next_node]._distance > 0) {
		int prev_node = pt->nodes[next_node]._prev_node;
		//if prev_node is the parent of next_node then the label to save is the next_nodes' label
		if (prev_node == pt->nodes[next_node].head) {
			path_id += d * (1 + pt->nodes[next_node].deprel);
			// else (prev_node is a child) then the label to save is the prev_node's labeld
		} else {
			path_id += d * (1 + mp->deprel_num + pt->nodes[prev_node].deprel);
		}
		d *= mp->deprel_num * 2 + 1;
		s++;
		next_node = prev_node;
	}
	ASSERT(s == l);
	ASSERT(path_id <= get_synt_path_feature_size(mp));

	return path_id;
}

