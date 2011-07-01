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

//COMMON CONSTS FOR DEP PARSING EXPERIMENTS

#ifndef _IDP_H
#define _IDP_H

#define _VERSION "0.0.2"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <values.h>
#include <time.h>
#include <gsl/gsl_linalg.h>

#include "idp_io_spec.h"
//DEF FUNCTIONS

#define CHECK_PTR(x) if ((x) == NULL) { printf("Can't allocate: %s:%d\n", __FILE__,  __LINE__); exit(-1);}
#define ALLOC(p, type) p = (type *) calloc(sizeof(type), 1); if ((p) == NULL) { printf("Can't allocate: %s:%d\n", __FILE__,  __LINE__); exit(-1);}
#define DEF_ALLOC(p, type) type *p = (type *) calloc(sizeof(type), 1); if ((p) == NULL) { printf("Can't allocate: %s:%d\n", __FILE__,  __LINE__); exit(-1);}
#define FREE(p) free(p)
#define ALLOC_ARRAY(p, type, size) p = (type *) calloc(sizeof(type), size); if ((p) == NULL) { printf("Can't allocate: %s:%d\n", __FILE__,  __LINE__); exit(-1);}
#define DEF_ALLOC_ARRAY(p, type, size) type *p = (type *) calloc(sizeof(type), size); if ((p) == NULL) { printf("Can't allocate: %s:%d\n", __FILE__,  __LINE__); exit(-1);}

#define DEF_FOPEN(fp, fname, access) \
         FILE *fp;\
         if (!(fp = fopen(fname, access))) { \
         fprintf(stderr, "Error: can't open file '%s'\n", fname); \
         exit(1); \
         }
#define FOPEN(fp, fname, access) \
         if (!(fp = fopen(fname, access))) { \
         fprintf(stderr, "Error: can't open file '%s'\n", fname); \
         exit(1); \
         }


#define ASSERT_ON 1
#if ASSERT_ON
#define ASSERT(x) if (!(x)) {  printf("Assertion failed: '%s':%d\n", __FILE__,  __LINE__);}
#else
#define ASSERT(x)
#endif

#define SQR(x)  ((x) * (x))
#define CUB(x)  ((x) * (x) * (x))
#define MAX(x,y) (((x) > (y)) ? (x) : (y));
#define BIN_RAND(mju) ((drand48() < (mju)) ? 1 : 0)

//maximum length of a record
#define MAX_REC_LEN 100


//CONSTANTS
#define MAX_HID_SIZE 80


#define MAX_BEAM 100


#define MAX_LINE 10000
#define MAX_WGT_LINE  100000

#define MAX_NAME 1000

#define SLICE_ALT_EPS 1.e-5
#define MJU_STEP_EPS 1.e-12

#define FF 1
#define MF 2


//to be used both in outputs and inputs
#define ACTION_NUM 15
#define SHIFT 0 //shift to SRL and SYN stacks (and predict word)
#define RED 1  //reduce from stack
#define LA 2 // left arc operation ( <--- + reduce from stack)
#define RA 3 // right arc operation
#define SWITCH 4 // switch from syntact to semantics parsing
#define FLIP 5   // flips elements on the stack for syntactic structures
#define PREDIC_YES 6  // predicate prediction
#define PREDIC_NO 7 // predict that not a predicate is on top of the stack
#define SRL_RED 8 // reduce from stack
extern const int SRL_LA[2];// = {9, 10}; //prob bank and nombank left arc
extern const int SRL_RA[2]; // = {11, 12}; // prob bank and nombank right arc
#define SRL_FLIP 13 // flips elements on the stack for SRL structures
#define SRL_SWITCH 14 // switch from semantics to syntactc parsing

#define ANY -1 //matches any operation

#define ROOT_DEPREL -1

#define FIELD_SEPS " \t\n\r"

#define BANK_NUM 2
#define PROP_BANK 0 // needs to be consistent with data
#define NOM_BANK 1  // needs to be consistent with data


//COMMON TYPES FOR DEP PARSING EXPERIMENTS

typedef struct layer {
    double mju[MAX_HID_SIZE];
    double  mju_err[MAX_HID_SIZE];
} LAYER;

typedef struct hh_links {
    double w[MAX_HID_SIZE][MAX_HID_SIZE];
    double w_del[MAX_HID_SIZE][MAX_HID_SIZE];
} HH_LINK;


typedef struct ih_biases {
    double b[MAX_HID_SIZE];
    double b_del[MAX_HID_SIZE];
} IH_LINK;


typedef struct out_link {
    double *b;   //[out]
    double *b_del; // [out]
    double **w;  //[out][hid]
    double **w_del; //[out][hid]

    //this attribute can be used to connect out_link with any associated info
    //(E.g. you might asociate smth with each link and
    //then you will be able to retrive it from corresponding option)
    void *info;
} OUT_LINK;


#define SYNT_STEP 0
#define SRL_STEP 1
#define ANY_STEP 2


#define MAX_LAB_PATH_LEN 1
#define PATH_FEATURE_CLASS 1
#define NODE_FEATURE_CLASS 2

#define DEPREL_TYPE 1
#define POS_TYPE 2
#define WORD_TYPE 3
#define FEAT_TYPE 4
#define CPOS_TYPE 5
#define LEMMA_TYPE 6
#define SENSE_TYPE 7

#define SYNT_LAB_PATH_TYPE 8

#define INP_SRC 1
#define STACK_SRC 2
#define SRL_STACK_SRC 3

#define USE_FILLPRED 1 /*use FILLPRED column in the corpus files*/

typedef struct node_route {
     int src;   //src of information: input string, stack or partial dep graph (_SRC)
    int offset_curr; //offset in current structure
    int offset_orig; //offset in original input
    int arg_bank, arg_role; //if bank positive - then a feature
    int head;  //number of head operations
    int rc; //number of rightmost child operations or leftmost (if negative)
    int rs; //number of right sibling applications or left sibling (if negative)
} NODE_ROUTE;

//an input feature specification, these features are used as  inputs to hidden layer
//specification approach is replicated from MALT parser
typedef struct inp_feature_spec {
    int step_type; //type of step (_STEP)
    int feature_class;  // feature class (PATH_FEATURE_CLASS/NODE_FEATURE_CLASS)
    int info_type;  //type of information (_TYPE)

    NODE_ROUTE node, node_other;
} IH_LINK_SPEC;

#define FIND_FIRST 1
#define FIND_LAST 2

//hidden to hidden connection specification
//it finds either last or first state matching given specification
typedef struct hh_links_spec {
    //either find first match (=leftmost) or the last one (=rightmost)
    int when;

    //take node A with the following properties
    int orig_step_type; // type of the original step (_STEP)
    int orig_src;   //src of original node: input or stack (_SRC)
    int orig_offset; //offset in original node
    int orig_head; //number of head operations
    int orig_arg_bank, orig_arg_role; //if bank positive - then  a feature
    //NB: NEXT OPERATIONS ARE NOT RC OPERATIONS: here, it  righmost RIGHT child (child_pos > head_pos)
    int orig_rrc; //number of rightmost right child or leftmost left child ops (if negative)
    int orig_rs; //number of right seebling applications or left seebling (if negative)

    //find state in the history with where A matches to the following node
    int target_step_type;
    int target_src;
    int target_offset;

    //next operation after match: SHIFT, RED, LA, RA and -1, which matches anything
    int act;

    //number of states before current (1 - previous, 2 ... - further in the history; -1 - any)
    int offset;

} HH_LINK_SPEC;



typedef struct outputs {
    //number of outputs
    int num;

    //output mask (1 for used, 0 for unused)
    double *mask; //[MAX_OUT_SIZE];
    int is_masked;


    //output activataions
    double *q;//[MAX_OUT_SIZE];
    //output error
    double *del;//[MAX_OUT_SIZE];

    // NB!!!: BE CAREFUL WITH parts, exp and norm fields -> this fields are actively used during
    // estimation of next mju (for next options)
    // they are not guaranted to be computed from this_opt->int_mju !!!!

    //parts under exponent in output activations
    double *parts;//[MAX_OUT_SIZE];
    //exponents in outputs activations
    double *exp;//[MAX_OUT_SIZE];

    //norm
    double norm;

    //when doing steps to find optimal mju[rho_idx] (f) :  exp(out[i]) = exp(a[i] * f + b[i]);
    double *a,//[MAX_OUT_SIZE]
           *b;//[MAX_OUT_SIZE];
} OUTPUTS;

typedef struct stack {
    int size;
    int elems[MAX_SENT_LEN];
} STACK;

typedef struct queue {
    int front, end;
    int elems[MAX_SENT_LEN];
} QUEUE;

typedef struct node {
    int child_num, head, deprel;
    //sorted from left to right
    int childs[MAX_SENT_LEN];

    // field for traversing: the previous node on the SST
    int _prev_node;
    int _distance;
} NODE;

typedef struct part_tree {
    int len;
    NODE *nodes;
    int right_connections_num[MAX_SENT_LEN + 1];
} PART_TREE;


typedef struct node_srl {
    int arg_num, head_num, sense;
    //sorted from let to right
    int args[MAX_ARG_NUM];
    //roles of childs
    int arg_roles[MAX_ARG_NUM];

    //parents of this token
    int heads[MAX_SENT_LEN];
    // argument roles in these predicates
    int my_roles[MAX_SENT_LEN];
} NODE_SRL;


typedef struct part_srl {
    int len;
    NODE_SRL *nodes;
    int right_connections_num[MAX_SENT_LEN + 1];
} PART_SRL;


#define STRUCT_DEC_OPT 1
#define POS_PRED_OPT 2
#define FEAT_PRED_OPT 3
#define WORD_PRED_OPT 4
#define LAB_PRED_OPT 5


#define MAX_IH_LINKS 170
//maximum number of distrinct (elementary) features for any word
#define MAX_SIMULT_FEAT 30

//bounded by MAX_SIMULT_FEAT * MAX_IH_LINKS,
#define MAX_IH_LINKS_OPT MAX_IH_LINKS + MAX_SIMULT_FEAT * 10


#define MAX_HH_LINKS 30



typedef struct option {
    int step_type;  // SRL_STEP or SYNT_STEP

    int state_idx;

    //log probability of preceeding steps
    double lprob;

    int type; // WORD_PRED_OPT or TAG_PRED_OPT
    int new_state;
    int previous_act;
    int previous_output;
    struct option *previous_option;
    //period - id of element in queue
    int period;

    struct option_list *next_options;

    //state means when conditioning only on the previous output (if any)
    LAYER int_layer;

    //is set only if new_option,
    //it defines mju for the previos state conditioned on all the decisions,
    //including one that led to this option
    LAYER prev_layer;

    double fw_means[MAX_HID_SIZE];

    //current option outputs
    OUTPUTS outputs;

    OUT_LINK *out_link;


    //links
    LAYER  *rel_layers[MAX_HH_LINKS];
    HH_LINK *rel_weights[MAX_HH_LINKS];
    int rel_num;

    //biases
    IH_LINK *inp_biases[MAX_IH_LINKS_OPT];
    int inp_num;

    STACK stack;
    STACK srl_stack;
    int queue;

    //stored only in the frontier options
    PART_TREE *pt;
    PART_SRL *ps;
} OPTION;

typedef struct option_list {
    struct option_list *next, *prev;
    OPTION *elem;
} OPTION_LIST;

typedef struct word_info {
    int num;;
} WORD_INFO;

typedef struct feat_info {
    int num;
    WORD_INFO word_infos[MAX_FEAT_INP_SIZE];
} FEAT_INFO;

typedef struct pos_info {
    int num;
    FEAT_INFO feat_infos[MAX_POS_INP_SIZE + 1];
} POS_INFO;

//information about the networks (weights, connection specification etc)
typedef struct model_params {

    //input links and input specification
    int ih_links_num;
    IH_LINK_SPEC ih_links_specs[MAX_IH_LINKS];
    IH_LINK
        **ih_link_cpos, //[link_id][cpos]
        **ih_link_deprel, //[link_id][deprel]
        **ih_link_pos, //[link_id][pos]
        **ih_link_lemma, //[link_id][lemma]
        **ih_link_elfeat, //[linnk_id][elfeat] - elementary features (decomposed)
        ***ih_link_feat, //[link_id][[pos][feat] - composed features
        ****ih_link_word, //[link_id][pos][feat][word]
        ****ih_link_sense,  //[link_id][bank][lemma][sense]
        **ih_link_synt_lab_path,   // [link_id][path_id]

        init_bias,
        hid_bias, hid_bias_srl,
        //for decision options
        prev_act_bias[ACTION_NUM],
        prev_deprel_bias[ACTION_NUM][MAX_DEPREL_SIZE],
        prev_arg_bias[ACTION_NUM][MAX_ROLE_SIZE];

    //hh links specifications and weights
    int hh_links_num;
    HH_LINK_SPEC hh_links_specs[MAX_HH_LINKS];
    HH_LINK *hh_link; //[link_id]

    //inp and output structure
    POS_INFO pos_info_out, pos_info_in;

    OUT_LINK
        out_link_act,
        out_link_la_label,
        out_link_ra_label,
        out_link_pos,
        *out_link_feat, //[pos]
        **out_link_word, //[pos][feat]

        *out_link_sem_la_label, // [bank]
        *out_link_sem_ra_label, // [bank]
        ***out_link_sense; //[bank][lemma][0];

    int cpos_num, deprel_num, lemma_num, elfeat_num;


    char s_deprel[MAX_DEPREL_SIZE][MAX_REC_LEN];
    char s_deprel_root[MAX_REC_LEN];

    // [bank][lemma] - give sense  num
    int **sense_num;
     //root id (can be used in not fully projective parsing)
    //int root_deprel;
    //  [bank][lemma][sense]
    char ****s_sense;

   // [bank] number of roles
    int role_num[BANK_NUM];
     // [bank][role]
    char s_arg_role[BANK_NUM][MAX_ROLE_SIZE][MAX_REC_LEN];



} MODEL_PARAMS;


typedef struct state {
    char where[MAX_NAME];
    double curr_reg, curr_eta;
    double max_score, last_score;
    double last_tr_err;
    int loops_num, num_sub_opt;

    //derivative of curr_reg and curr_eta (not to be saved)
    double decay;

} STATE;

#define DD_FISHER_KERNEL 0
#define DD_TOP_KERNEL 1

#define FEAT_MODE_COMPOSED 0
#define FEAT_MODE_ELEMENTARY 1
#define FEAT_MODE_BOTH 2


#define DEPROJ_NO 0
#define DEPROJ_GREEDY 1
#define DEPROJ_EXHAUSTIVE 2

typedef struct approx_params {
    int approx_type; //FF || MF
    int beam, search_br_factor;
    int seed;
    double rand_range, em_rand_range;

    int hid_size;

    //should candidates be returned and saved; cand_num can not be large  than search_br_factor
    int return_cand;
    int cand_num;

    double init_eta, eta_red_rate, max_eta_red;
    double mom;
    double init_reg, reg_red_rate, max_reg_red;
    int max_loops, max_loops_wo_accur_impr, loops_between_val;

    //current state
    STATE st;

    char train_file[MAX_NAME];
    //only for data sampling
    int train_dataset_size;
    char test_file[MAX_NAME];

    //only for data sampling
    int test_dataset_size;
    char model_name[MAX_NAME];


    //a file to write output to
    char out_file[MAX_NAME];

    //specification file (output/input specification file)
    char io_spec_file[MAX_NAME];

    //feature (input link definition file)
    int inp_feat_mode; //set to FEAT_MODE_*

    char ih_links_spec_file[MAX_NAME];

    //specification of hid-hid links
    char hh_links_spec_file[MAX_NAME];

    //shift of the input: if 0: stack top is shifted, 1 - first in queue is shifted also
    int input_offset;

    //parse mode (-o option of MALT parser)
    int parsing_mode;

    // if we should perform early reduction
    int is_synt_early_reduce;
    int is_srl_early_reduce;
    // if we should deprojectivize internally
    int intern_srl_deproj;
    int intern_synt_deproj;

    // if set biases for syntactic and semantic operiations should be distinguished
    int disting_biases;

    //components in reranker to be used
    int use_dd_comp, use_expl_comp;
    double lprob_w, dd_w;

    int dd_kernel_type;

    double mbr_coeff;

} APPROX_PARAMS;



//NB: it starts from index 1. 'len' counts only meaningful elements
//So, len + 1 elements in total
typedef struct sentence {
    int len;
    char s_word[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int  word_in[MAX_SENT_LEN + 2], word_out[MAX_SENT_LEN + 2];

    char s_lemma[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int lemma[MAX_SENT_LEN + 2];

    char s_cpos[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int cpos[MAX_SENT_LEN + 2];

    char s_pos[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int pos[MAX_SENT_LEN + 2];

    char s_feat[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int feat_in[MAX_SENT_LEN + 2], feat_out[MAX_SENT_LEN + 2], elfeat_len[MAX_SENT_LEN + 2], elfeat[MAX_SENT_LEN + 2][MAX_SIMULT_FEAT];

    int head[MAX_SENT_LEN + 2];

    char s_deprel[MAX_SENT_LEN + 2][MAX_REC_LEN];
    int  deprel[MAX_SENT_LEN + 2];

    int fill_pred[MAX_SENT_LEN + 2]; //1 if the pred is filled and 0 otherwise

    //treebank PROP_BANK or NOM_BANK  or -1
    int  bank[MAX_SENT_LEN + 2];

    char s_sense[MAX_SENT_LEN+2][MAX_REC_LEN];
    int sense[MAX_SENT_LEN + 2];

    int arg_num[MAX_SENT_LEN + 1];
    int args[MAX_SENT_LEN + 1][MAX_ARG_NUM];
    char s_arg_roles[MAX_SENT_LEN + 1][MAX_ARG_NUM][MAX_REC_LEN];
    int arg_roles[MAX_SENT_LEN + 1][MAX_ARG_NUM];

    // for each word contains number of arcs connecting to the right in the SRL structure
    int srl_right_degree[MAX_SENT_LEN + 1];
    // for each word and each connection to the right contains index of the connection (sorted) in the SRL structure
    int srl_right_indices[MAX_SENT_LEN + 1][MAX_SENT_LEN + 1];

    // the same as above but for syntax
    int synt_right_degree[MAX_SENT_LEN + 1];
    int synt_right_indices[MAX_SENT_LEN + 1][MAX_SENT_LEN + 1];

} SENTENCE;


typedef struct action_set {
    //set to one if possible
    int acts[ACTION_NUM];
} ACTION_SET;

typedef struct eval {
    int model, gold, matched;
} EVAL;


#define PL_STARTED "STARTED"
#define PL_TRAIN_LOOP_END "TRAIN_LOOP_END"
#define PL_VAL_END "VAL_END"
#define PL_FINISHED "FINISHED"

//METHODS

//******************************
//***  idp_common_api.c
//******************************

double sigmoid(double x);
int sample(int n, double *distrib);

//*******************************
//***  idp_data_manup.c
//*******************************

//reads and reserves space in mp for corresponding weights
//int read_ih_links_spec(char *fname, MODEL_PARAMS *mp);
int  read_ih_links_spec(char *fname, MODEL_PARAMS *mp, IH_LINK_SPEC *specs, int *num);
int read_hh_links_spec(char *fname, MODEL_PARAMS *mp);

//reads IO specification file
void read_io_spec(char *fname, MODEL_PARAMS *mp);

//allocates memory space for all the weights
//should be invoked after reading hh, ih and io specs
void allocate_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp);
void free_weights(MODEL_PARAMS *mp);

void load_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname);
void save_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname);
void load_weight_dels(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname);
void save_weight_dels(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname);
void sample_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp);

char *print_sent(SENTENCE *sent, int with_links);
void append_sentence(char *fname, SENTENCE *sen, int with_links);
SENTENCE *read_sentence(MODEL_PARAMS *mp, FILE *fp, int with_links);
int is_blind_file(char *fname);
void save_sentence(FILE *fp, SENTENCE *sen, int with_links);
void save_candidates(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *inp_sent, OPTION **best_options, int avail_num);
SENTENCE *create_blind_sent(SENTENCE *gld_sent);


int get_matched_syntax(SENTENCE *sent1, SENTENCE *sent2, int labeled);
void add_matched_srl(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval);
void add_matched_roles(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval);
void add_matched_predicates(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval);
void add_matched_bank_roles(int bank, SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval);
void add_matched_bank_predicates(int bank, SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval);


// if a > b return positive, a == b - return 0, else return negative
int lexicograph_ignore_double_entries_comp(int a[], int a_len, int b[], int b_len);


char *get_params_string(APPROX_PARAMS *ap);
APPROX_PARAMS *read_params(char *fname);

void print_state(char *s, APPROX_PARAMS *ap);
void save_state(APPROX_PARAMS *ap);
int load_state(APPROX_PARAMS *ap);

void err_chk(void *p, char* msg);
char *field_missing_wrapper(char* s, char *name);


//*******************************
//***  idp_queue.c
//*******************************
void q_init(QUEUE *q);
int q_look(QUEUE *q, int i);
int q_peek(QUEUE *q);
int q_pop(QUEUE *q);
void q_push(QUEUE *q, int x);


//*******************************
//***  idp_stack.c
//*******************************
int st_look(STACK *st, int i);
int st_peek(STACK *st);
int st_pop(STACK *st);
void st_push(STACK *st, int x);


//*******************************
//***  idp_part_tree.c
//*******************************

PART_TREE *pt_alloc(int sent_len);
PART_TREE *pt_clone(PART_TREE *pt);
void pt_free(PART_TREE **pt);
int pt_lc(PART_TREE *pt, int i);
int pt_llc(PART_TREE *pt, int i);
int pt_rc(PART_TREE *pt, int i);
int pt_rrc(PART_TREE *pt, int i);
int pt_ls(PART_TREE *pt, int i);
int pt_rs(PART_TREE *pt, int i);
void pt_add_link(PART_TREE *pt, int h, int d, int deprel);
void pt_fill_sentence(PART_TREE *pt, MODEL_PARAMS *mp, SENTENCE *sent);
int pt_is_head_of(PART_TREE *pt, int h, int a);

#define pt_head(pt, i)  (pt)->nodes[i].head
#define pt_deprel(pt,i) (pt)->nodes[i].deprel

//returns undericted path lentght between to nodes or -0 if no existrs
// path is marked by _prev_node labels in NODES (if > 0), search back from j
int pt_get_nondirected_path(PART_TREE  *pt, int i, int j, int max_distance);
int pt_get_degree(PART_TREE *pt, int w);


//*******************************
//***  idp_part_srl.c
//*******************************
PART_SRL *ps_alloc(int sent_len);
PART_SRL *ps_clone(PART_SRL *ps);
int ps_arg(PART_SRL *ps, int i, int role);
void ps_free(PART_SRL **ps);
int ps_is_head_of(PART_SRL *ps, int h, int a);
int is_ps_role(PART_SRL *ps, int h, int a, int r);
void ps_set_sense(PART_SRL *ps, int i, int sense);
void ps_add_link(PART_SRL *ps, int h, int d, int role);
void ps_fill_sentence(PART_SRL *ps, MODEL_PARAMS *mp, SENTENCE *sent);
#define ps_sense(ps, i)  (ps)->nodes[i].sense
int ps_get_degree(PART_SRL *ps, int w);

//*******************************
//***  idp_parse.c
//*******************************
//processes sentence and creates derivations
//returns tail of the derivation
OPTION *get_derivation(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, int do_free_partials);
//the same but puts the options in the list
OPTION *get_derivation_list(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent,
        OPTION_LIST **der_first, OPTION_LIST **der_last, int do_free_partials);
OPTION * create_word_prediction_seq(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent, OPTION *opt, int do_free_partials);
OPTION *create_option_light(int step_type, int new_state, int previous_act, int previous_output, OPTION *previous_option, int period, STACK *stack,  STACK *srl_stack,
        int queue, PART_TREE *pt,  PART_SRL *ps,  OUT_LINK *out_link, int out_num);
void alloc_option_outputs(OPTION *opt);
OPTION *create_option(int step_type, int new_state, int previous_act, int previous_output, OPTION *previous_option, int period, STACK *stack,  STACK *srl_stack,
        int queue, PART_TREE *pt,  PART_SRL *ps,  OUT_LINK *out_link, int out_num);
void set_links(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *sent);
ACTION_SET get_next_actions(APPROX_PARAMS *ap, MODEL_PARAMS *mp, OPTION *opt, SENTENCE *s);
void set_mask(MODEL_PARAMS *mp, OPTION *opt, ACTION_SET *as);

int get_word_by_route(OPTION *opt, NODE_ROUTE *spec, int spec_step_type);
int get_prev_deprel(MODEL_PARAMS *mp, OPTION *opt);
int get_prev_arg_role(MODEL_PARAMS *mp, OPTION *opt);

int  get_synt_path_feature_size(MODEL_PARAMS *mp);
int get_synt_path_feature_id(MODEL_PARAMS *mp, OPTION *opt, int w, int w_other);


//*******************************
//***  idp_dyn_struct.c
//*******************************
OPTION_LIST *increase_list(OPTION_LIST *prev_node, OPTION *opt);
OPTION_LIST *increase_list_asc(OPTION_LIST *tail, OPTION *opt);
OPTION_LIST *reduce_list(OPTION_LIST *last_node);
void free_option_list(OPTION_LIST *last);
void free_option(OPTION *opt);
void free_options(OPTION *last_opt);
void free_derivation(OPTION_LIST *last);
OPTION_LIST *concat(OPTION_LIST *ol1, OPTION_LIST *ol2);
OPTION_LIST *prune_options(OPTION_LIST *last_opt, double threshold_lprob);
int prune_free(OPTION **best_options, OPTION_LIST *tail, int beam);


//*******************************
//*** idp_frame.c
//*******************************

double testing_epoch(APPROX_PARAMS *p, MODEL_PARAMS *w);
double training_epoch(APPROX_PARAMS *p, MODEL_PARAMS *w);
//double label_sequence(APPROX_PARAMS *ap, MODEL_PARAMS *w, SEQUENCE *seq);
void validation_training(APPROX_PARAMS *ap, MODEL_PARAMS *mp);

//*******************************
//*** idp_estim.c
//*******************************

double comp_sentence(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *head);
void comp_sentence_del(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *der_last);
void update_weights(APPROX_PARAMS *ap, MODEL_PARAMS *w, int sent);
void comp_option_states(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt);
void comp_option_outputs(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt);
void comp_option(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt);
void fill_lprob(OPTION *opt);

//*******************************
//*** idp_search.c
//*******************************

double label_sentence(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *sent);

//*******************************
//*** idp_main.c
//*******************************
void rewrite_parameters(APPROX_PARAMS *ap, int argc, char** argv, int start_idx);


#endif
