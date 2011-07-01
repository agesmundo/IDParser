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

#define PROGRESSDOT 0 /* print a "." after reading each sentence. */


//exception handlers
char *field_missing_wrapper(char* s, char *name) {
	if (s == NULL) {
		fprintf(stderr, "Error: field %s is missing\n", name);
		exit(1);
	}
	return s;
}
char *field_missing_wrapper_ext(char* s, char *name, char *file_type) {
	if (s == NULL) {
		fprintf(stderr, "Error: field %s is missing in file '%s'\n", name, file_type);
		exit(1);
	}
	return s;
}
void err_chk(void *p, char* msg) {
	if (p == NULL) {
		fprintf(stderr, "Error: %s\n", msg);
		exit(1);
	}
}


void alloc_out_link_contents(OUT_LINK *link, int out_size, int hid_size) {
	ALLOC_ARRAY(link->b, double, out_size);
	ALLOC_ARRAY(link->b_del, double,  out_size);
	ALLOC_ARRAY(link->w, double*, out_size);
	ALLOC_ARRAY(link->w_del, double*, out_size);
	int o;
	for (o = 0; o < out_size; o++) {
		ALLOC_ARRAY(link->w[o], double, hid_size);
		ALLOC_ARRAY(link->w_del[o], double, hid_size);
	}

}
void free_out_link_contents(OUT_LINK *link, int out_size) {
	free(link->b);
	free(link->b_del);
	int o;
	for (o = 0; o < out_size; out_size++) {
		free(link->w[o]);
		free(link->w_del[o]);
	}
	free(link->w);
	free(link->w_del);
}

#define RAND_WEIGHT ( (ap->rand_range)*2.0*drand48()-(ap->rand_range))
#define RAND_EMISS_WEIGHT ((ap->em_rand_range)*2.0*drand48()-(ap->em_rand_range))
//loads parameters
void sample_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {
	//sample input links
	int l;

	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			int i, h;
			//+ 1  to represent the fact that there is
			//no word complying to the specification l
			for (i = 0; i < mp->cpos_num + 1; i++) {
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_cpos[l][i].b[h] = RAND_WEIGHT;
				}
			}
		} else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			int i,h;
			// +2 : for ROOT relationship and to represent fact that there is
			// not word which complies with the specification l
			for (i = 0; i < mp->deprel_num + 2; i++) {
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_deprel[l][i].b[h] = RAND_WEIGHT;
				}
			}
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			int i,h;
			for (i = 0; i < mp->pos_info_in.num + 1; i++) {
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_pos[l][i].b[h] = RAND_WEIGHT;
				}
			}
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			int i,h;
			for (i = 0; i < mp->lemma_num + 1; i++) {
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_lemma[l][i].b[h] = RAND_WEIGHT;
				}
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			int b, lemma, s, h;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					for (s = 0; s < mp->sense_num[b][lemma]; s++) {
						for (h = 0; h < ap->hid_size; h++) {
							mp->ih_link_sense[l][b][lemma][s].b[h] = RAND_WEIGHT;
						}
					}
				}
			}
			//link for no word
			for (b = BANK_NUM; b < BANK_NUM + 2; b++) {
				for (h  = 0; h < ap->hid_size; h++) {
					mp->ih_link_sense[l][b][0][0].b[h] = RAND_WEIGHT;
				}
			}
		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int p, f, h;
				for (p = 0; p < mp->pos_info_in.num; p++) {
					for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
						for (h = 0; h < ap->hid_size; h++) {
							mp->ih_link_feat[l][p][f].b[h] = RAND_WEIGHT;
						}
					}
				}
				//link for no word
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_feat[l][mp->pos_info_in.num][0].b[h] = RAND_WEIGHT;
				}
			}
			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int i, h;
				for (i = 0; i < mp->elfeat_num + 1; i++) {
					for (h = 0; h < ap->hid_size; h++) {
						mp->ih_link_elfeat[l][i].b[h] = RAND_WEIGHT;
					}
				}
			}

		} else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
			int p, f, w, h;
			for (p = 0; p < mp->pos_info_in.num; p++) {
				for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
					for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
						for (h = 0; h < ap->hid_size; h++) {
							mp->ih_link_word[l][p][f][w].b[h] = RAND_WEIGHT;
						}
					}
				}
			}
			//link for no word
			for (h = 0; h < ap->hid_size; h++) {
				mp->ih_link_word[l][mp->pos_info_in.num][0][0].b[h] = RAND_WEIGHT;
			}
		} else if (mp->ih_links_specs[l].info_type ==  SYNT_LAB_PATH_TYPE) {
			int path_num = get_synt_path_feature_size(mp);
			int p, h;
			for (p = 0; p < path_num + 1; p++) {
				for (h = 0; h < ap->hid_size; h++) {
					mp->ih_link_synt_lab_path[l][p].b[h] = RAND_WEIGHT;
				}
			}
		} else {
			ASSERT(0);
		}
	}

	int h;
	for (h = 0; h < ap->hid_size; h++) {
		mp->init_bias.b[h] = RAND_WEIGHT;
		mp->hid_bias.b[h] = RAND_WEIGHT;
		mp->hid_bias_srl.b[h] = RAND_WEIGHT;
	}

	int a;
	for (a = 0; a < ACTION_NUM; a++) {
		for (h = 0; h < ap->hid_size; h++) {
			mp->prev_act_bias[a].b[h] = RAND_WEIGHT;
		}
		int d;
		for (d = 0; d < mp->deprel_num; d++) {
			for (h = 0; h < ap->hid_size; h++) {
				mp->prev_deprel_bias[a][d].b[h] = RAND_WEIGHT;
			}
		}
		int b;
		int max = 0;
		for (b = 0; b < BANK_NUM; b++) {
			if (mp->role_num[b] > max) {
				max = mp->role_num[b];
			}
		}
		for (d = 0; d < max; d++) {
			for (h = 0; h < ap->hid_size; h++) {
				mp->prev_arg_bias[a][d].b[h] = RAND_WEIGHT;
			}
		}
	}


	//sample HID-HID weight matrixes
	int h1, h2;
	for (l = 0; l < mp->hh_links_num; l++) {
		for (h1 = 0; h1 < ap->hid_size; h1++) {
			for (h2 = 0; h2 < ap->hid_size; h2++) {
				mp->hh_link[l].w[h1][h2] = RAND_WEIGHT;
			}
		}
	}

	//sample output weights
#define SAMPLE_OUT_LINK(out_link) \
	{\
	(out_link).b[t] = RAND_EMISS_WEIGHT;\
	for (i = 0; i < ap->hid_size; i++) {\
		(out_link).w[t][i] = RAND_EMISS_WEIGHT;\
	}\
	}


	int t, i;
	for (t = 0; t < ACTION_NUM; t++) {
		SAMPLE_OUT_LINK(mp->out_link_act);
	}

	for (t = 0; t < mp->deprel_num; t++) {
		SAMPLE_OUT_LINK(mp->out_link_la_label);
		SAMPLE_OUT_LINK(mp->out_link_ra_label);
	}

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	int pos_num = ap->input_offset + mp->pos_info_out.num;
	for (t = 0; t < pos_num; t++) {
		SAMPLE_OUT_LINK(mp->out_link_pos);
	}
	int p;
	for (p = 0; p < pos_num; p++) {
		for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
			SAMPLE_OUT_LINK(mp->out_link_feat[p]);
		}
	}

	int f;
	for (p = 0; p < pos_num; p++) {
		for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
			for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
				SAMPLE_OUT_LINK(mp->out_link_word[p][f]);
			}
		}
	}

	int b;
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			SAMPLE_OUT_LINK(mp->out_link_sem_la_label[b]);
			SAMPLE_OUT_LINK(mp->out_link_sem_ra_label[b]);
		}
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			for (t = 0; t < mp->sense_num[b][l]; t++) {
				SAMPLE_OUT_LINK(mp->out_link_sense[b][l][0]);
			}
		}

	}
}


//loads parameters
void load_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname) {

	FILE *fp;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}

	char buffer[MAX_WGT_LINE];

#define READ_IH_WEIGHTS(ih, name)\
	{\
	do { \
		err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); \
	} while (buffer[0] == '#'); \
	(ih).b[0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), name));\
	for (h = 1; h < ap->hid_size; h++) { \
		(ih).b[h] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name)); \
	}\
	}

	//read input links
	int l;

	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			int i, h;
			//+ 1  to represent the fact that there is
			//no word complying to the specification l
			for (i = 0; i < mp->cpos_num + 1; i++) {
				READ_IH_WEIGHTS(mp->ih_link_cpos[l][i], "ih_link_cpos");
			}
		} else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			int i,h;
			// +2 : for ROOT relationship and to represent fact that there is
			// not word which complies with the specification l
			for (i = 0; i < mp->deprel_num + 2; i++) {
				READ_IH_WEIGHTS(mp->ih_link_deprel[l][i],"ih_link_deprel");
			}
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			int i,h;
			for (i = 0; i < mp->pos_info_in.num + 1; i++) {
				READ_IH_WEIGHTS(mp->ih_link_pos[l][i], "ih_link_pos");
			}
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			int i,h;
			for (i = 0; i < mp->lemma_num + 1; i++) {
				READ_IH_WEIGHTS(mp->ih_link_lemma[l][i], "ih_link_lemma");
			}
		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int p, f, h;
				for (p = 0; p < mp->pos_info_in.num; p++) {
					for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
						READ_IH_WEIGHTS(mp->ih_link_feat[l][p][f], "ih_link_feat");
					}
				}
				//link for no word
				READ_IH_WEIGHTS(mp->ih_link_feat[l][mp->pos_info_in.num][0], "ih_link_feat");
			}
			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int i, h;
				for (i = 0; i < mp->elfeat_num + 1; i++) {
					READ_IH_WEIGHTS(mp->ih_link_elfeat[l][i], "ih_link_elfeat");
				}
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			int b, lemma, s, h;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					for (s = 0; s < mp->sense_num[b][lemma]; s++) {
						READ_IH_WEIGHTS(mp->ih_link_sense[l][b][lemma][s], "ih_link_sense");
					}
				}
			}
			READ_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM][0][0], "ih_link_sense");
			READ_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM + 1][0][0], "ih_link_sense");
		} else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
			int p, f, w, h;
			for (p = 0; p < mp->pos_info_in.num; p++) {
				for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
					for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
						READ_IH_WEIGHTS(mp->ih_link_word[l][p][f][w], "ih_link_word");
					}
				}
			}
			//link for no word
			READ_IH_WEIGHTS(mp->ih_link_word[l][mp->pos_info_in.num][0][0], "ih_link_word");
		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			int i,h;
			int path_num = get_synt_path_feature_size(mp);
			for (i = 0; i < path_num + 1; i++) {
				READ_IH_WEIGHTS(mp->ih_link_synt_lab_path[l][i], "ih_link_path");
			}
		} else {
			ASSERT(0);
		}
	}

	int h;
	READ_IH_WEIGHTS(mp->init_bias, "init_bias");
	READ_IH_WEIGHTS(mp->hid_bias, "hid_bias");
	READ_IH_WEIGHTS(mp->hid_bias_srl, "hid_bias_srl");

	int a;
	for (a = 0; a < ACTION_NUM; a++) {
		READ_IH_WEIGHTS(mp->prev_act_bias[a], "prev_act_bias");
	}

	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < mp->deprel_num; d++) {
			READ_IH_WEIGHTS(mp->prev_deprel_bias[a][d], "prev_deprel_bias");
		}
	}
	int b;
	int max = 1;
	for (b = 0; b < BANK_NUM; b++) {
		if (mp->role_num[b] > max) {
			max = mp->role_num[b];
		}
	}
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < max; d++) {
			READ_IH_WEIGHTS(mp->prev_arg_bias[a][d], "prev_arg_bias");
		}
	}

	//load HID-HID weight matrixes
	int h1, h2;
	for (l = 0; l < mp->hh_links_num; l++) {
		for (h1 = 0; h1 < ap->hid_size; h1++) {
			do {
				err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights");
			} while (buffer[0] == '#');
			mp->hh_link[l].w[h1][0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), "hh_link"));
			for (h2 = 1; h2 < ap->hid_size; h2++) {
				mp->hh_link[l].w[h1][h2] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), "hh_link"));
			}
		}
	}

	//load output weights
#define READ_OUT_LINK(out_link, name)\
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
		READ_OUT_LINK(mp->out_link_act, "out_link_act");
	}

	for (t = 0; t < mp->deprel_num; t++) {
		READ_OUT_LINK(mp->out_link_la_label, "out_link_la_label");
		READ_OUT_LINK(mp->out_link_ra_label, "out_link_ra_label");
	}

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	int pos_num = ap->input_offset + mp->pos_info_out.num;
	for (t = 0; t < pos_num; t++) {
		READ_OUT_LINK(mp->out_link_pos, "out_link_pos");
	}
	int p;
	for (p = 0; p < pos_num; p++) {
		for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
			READ_OUT_LINK(mp->out_link_feat[p], "out_link_feat");
		}
	}

	int f;
	for (p = 0; p < pos_num; p++) {
		for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
			for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
				READ_OUT_LINK(mp->out_link_word[p][f], "out_link_word");
			}
		}
	}

	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			READ_OUT_LINK(mp->out_link_sem_la_label[b], "out_link_sem_la_label");
		}
	}
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			READ_OUT_LINK(mp->out_link_sem_ra_label[b], "out_link_sem_ra_label");
		}
	}


	for (b = 0; b < BANK_NUM; b++) {
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			for (t = 0; t < mp->sense_num[b][l]; t++) {
				READ_OUT_LINK(mp->out_link_sense[b][l][0], "out_link_sense");
			}
		}
	}


	fclose(fp);

}
//loads weight deltas
void load_weight_dels(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname) {

	FILE *fp;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}

	char buffer[MAX_WGT_LINE];

#define READ_IH_WEIGHT_DELS(ih, name)\
	{\
	do { \
		err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); \
	} while (buffer[0] == '#'); \
	(ih).b_del[0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), name));\
	for (h = 1; h < ap->hid_size; h++) { \
		(ih).b_del[h] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name)); \
	}\
	}

	//read input links
	int l;

	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			int i, h;
			//+ 1  to represent the fact that there is
			//no word complying to the specification l
			for (i = 0; i < mp->cpos_num + 1; i++) {
				READ_IH_WEIGHT_DELS(mp->ih_link_cpos[l][i], "ih_link_cpos");
			}
		} else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			int i,h;
			// +2 : for ROOT relationship and to represent fact that there is
			// not word which complies with the specification l
			for (i = 0; i < mp->deprel_num + 2; i++) {
				READ_IH_WEIGHT_DELS(mp->ih_link_deprel[l][i],"ih_link_deprel");
			}
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			int i,h;
			for (i = 0; i < mp->pos_info_in.num + 1; i++) {
				READ_IH_WEIGHT_DELS(mp->ih_link_pos[l][i], "ih_link_pos");
			}
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			int i,h;
			for (i = 0; i < mp->lemma_num + 1; i++) {
				READ_IH_WEIGHT_DELS(mp->ih_link_lemma[l][i], "ih_link_lemma");
			}
		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int p, f, h;
				for (p = 0; p < mp->pos_info_in.num; p++) {
					for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
						READ_IH_WEIGHT_DELS(mp->ih_link_feat[l][p][f], "ih_link_feat");
					}
				}
				//link for no word
				READ_IH_WEIGHT_DELS(mp->ih_link_feat[l][mp->pos_info_in.num][0], "ih_link_feat");
			}

			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int i, h;
				for (i = 0; i < mp->elfeat_num + 1; i++) {
					READ_IH_WEIGHT_DELS(mp->ih_link_elfeat[l][i], "ih_link_elfeat");
				}
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			int b, lemma, s, h;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					for (s = 0; s < mp->sense_num[b][lemma]; s++) {
						READ_IH_WEIGHT_DELS(mp->ih_link_sense[l][b][lemma][s], "ih_link_sense");
					}
				}
			}
			READ_IH_WEIGHT_DELS(mp->ih_link_sense[l][BANK_NUM][0][0], "ih_link_sense");
			READ_IH_WEIGHT_DELS(mp->ih_link_sense[l][BANK_NUM + 1][0][0], "ih_link_sense");
		} else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
			int p, f, w, h;
			for (p = 0; p < mp->pos_info_in.num; p++) {
				for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
					for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
						READ_IH_WEIGHT_DELS(mp->ih_link_word[l][p][f][w], "ih_link_word");
					}
				}
			}
			//link for no word
			READ_IH_WEIGHT_DELS(mp->ih_link_word[l][mp->pos_info_in.num][0][0], "ih_link_word");
		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			int i,h;
			int path_num = get_synt_path_feature_size(mp);
			for (i = 0; i < path_num + 1; i++) {
				READ_IH_WEIGHT_DELS(mp->ih_link_synt_lab_path[l][i], "ih_link_synt_lab_path");
			}
		} else {
			ASSERT(0);
		}
	}

	int h;
	READ_IH_WEIGHT_DELS(mp->init_bias, "init_bias");
	READ_IH_WEIGHT_DELS(mp->hid_bias, "hid_bias");
	READ_IH_WEIGHT_DELS(mp->hid_bias_srl, "hid_bias_srl");

	int a;
	for (a = 0; a < ACTION_NUM; a++) {
		READ_IH_WEIGHT_DELS(mp->prev_act_bias[a], "prev_act_bias");
	}

	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < mp->deprel_num; d++) {
			READ_IH_WEIGHT_DELS(mp->prev_deprel_bias[a][d], "prev_deprel_bias");
		}
	}
	int b;
	int max = 1;
	for (b = 0; b < BANK_NUM; b++) {
		if (mp->role_num[b] > max) {
			max = mp->role_num[b];
		}
	}
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < max; d++) {
			READ_IH_WEIGHT_DELS(mp->prev_arg_bias[a][d], "prev_arg_bias");
		}
	}

	//load HID-HID weight matrixes
	int h1, h2;
	for (l = 0; l < mp->hh_links_num; l++) {
		for (h1 = 0; h1 < ap->hid_size; h1++) {
			do {
				err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights");
			} while (buffer[0] == '#');
			mp->hh_link[l].w_del[h1][0] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), "hh_link"));
			for (h2 = 1; h2 < ap->hid_size; h2++) {
				mp->hh_link[l].w_del[h1][h2] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), "hh_link"));
			}
		}
	}

	//load output weights
#define READ_OUT_LINK_DEL(out_link, name)\
	{\
	do { \
		err_chk(fgets(buffer, MAX_WGT_LINE, fp), "Problem with reading weights"); \
	} while (buffer[0] == '#'); \
	(out_link).b_del[t] = atof(field_missing_wrapper(strtok(buffer, FIELD_SEPS), name));\
	for (i = 0; i < ap->hid_size; i++) { \
		(out_link).w_del[t][i] = atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name)); \
	}\
	}


	int t, i;
	for (t = 0; t < ACTION_NUM; t++) {
		READ_OUT_LINK_DEL(mp->out_link_act, "out_link_act");
	}

	for (t = 0; t < mp->deprel_num; t++) {
		READ_OUT_LINK_DEL(mp->out_link_la_label, "out_link_la_label");
		READ_OUT_LINK_DEL(mp->out_link_ra_label, "out_link_ra_label");
	}

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	int pos_num = ap->input_offset + mp->pos_info_out.num;
	for (t = 0; t < pos_num; t++) {
		READ_OUT_LINK_DEL(mp->out_link_pos, "out_link_pos");
	}
	int p;
	for (p = 0; p < pos_num; p++) {
		for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
			READ_OUT_LINK_DEL(mp->out_link_feat[p], "out_link_feat");
		}
	}

	int f;
	for (p = 0; p < pos_num; p++) {
		for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
			for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
				READ_OUT_LINK_DEL(mp->out_link_word[p][f], "out_link_word");
			}
		}
	}

	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			READ_OUT_LINK_DEL(mp->out_link_sem_la_label[b], "out_link_sem_la_label");
		}
	}
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			READ_OUT_LINK_DEL(mp->out_link_sem_ra_label[b], "out_link_sem_ra_label");
		}
	}


	for (b = 0; b < BANK_NUM; b++) {
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			for (t = 0; t < mp->sense_num[b][l]; t++) {
				READ_OUT_LINK_DEL(mp->out_link_sense[b][l][0], "out_link_sense");
			}
		}
	}



	fclose(fp);

}
//saves parameters
void save_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname) {

	FILE *fp;
	if ((fp = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}


#define SAVE_IH_WEIGHTS(ih)\
	{\
	fprintf(fp, "%.20f", (ih).b[0]); \
	for (h = 1; h < ap->hid_size; h++) { \
		fprintf(fp, " %.20f", (ih).b[h]); \
	}\
	fprintf(fp, "\n");\
	}

	//save input links
	int l;
	fprintf(fp, "#ih_links\n#======================\n");
	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			int i, h;
			//+ 1  to represent the fact that there is
			//no word complying to the specification l
			fprintf(fp, "#Link %d, CPOS_TYPE\n", l);
			for (i = 0; i < mp->cpos_num + 1; i++) {
				SAVE_IH_WEIGHTS(mp->ih_link_cpos[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, DEPREL_TYPE\n", l);
			// +2 : for ROOT relationship and to represent fact that there is
			// not word which complies with the specification l
			for (i = 0; i < mp->deprel_num + 2; i++) {
				SAVE_IH_WEIGHTS(mp->ih_link_deprel[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, POS_TYPE\n", l);
			for (i = 0; i < mp->pos_info_in.num + 1; i++) {
				SAVE_IH_WEIGHTS(mp->ih_link_pos[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, LEMMA_TYPE\n", l);
			for (i = 0; i < mp->lemma_num + 1; i++) {
				SAVE_IH_WEIGHTS(mp->ih_link_lemma[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int p, f, h;
				fprintf(fp, "#Link %d, FEAT_TYPE (COMPOSED)\n", l);
				for (p = 0; p < mp->pos_info_in.num; p++) {
					for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
						SAVE_IH_WEIGHTS(mp->ih_link_feat[l][p][f]);
					}
				}
				//link for no word
				SAVE_IH_WEIGHTS(mp->ih_link_feat[l][mp->pos_info_in.num][0]);
			}
			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int i, h;
				fprintf(fp, "#Link %d, FEAT_TYPE (ELEMENTARY)\n", l);
				for (i = 0; i < mp->elfeat_num + 1; i++) {
					SAVE_IH_WEIGHTS(mp->ih_link_elfeat[l][i]);
				}
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			fprintf(fp, "#Link %d, SENSE_TYPE\n", l);
			int b, lemma, s, h;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					for (s = 0; s < mp->sense_num[b][lemma]; s++) {
						SAVE_IH_WEIGHTS(mp->ih_link_sense[l][b][lemma][s]);
					}
				}
			}
			//link for no word and no sense of the word
			SAVE_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM][0][0]);
			SAVE_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM + 1][0][0]);
		} else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
			fprintf(fp, "#Link %d, WORD_TYPE\n", l);
			int p, f, w, h;
			for (p = 0; p < mp->pos_info_in.num; p++) {
				for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
					for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
						SAVE_IH_WEIGHTS(mp->ih_link_word[l][p][f][w]);
					}
				}
			}
			//link for no word
			SAVE_IH_WEIGHTS(mp->ih_link_word[l][mp->pos_info_in.num][0][0]);
		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, SYNT_LAB_PATH_TYPE\n", l);
			int path_num = get_synt_path_feature_size(mp);
			for (i = 0; i < path_num + 1; i++) {
				SAVE_IH_WEIGHTS(mp->ih_link_synt_lab_path[l][i]);
			}
		} else {
			ASSERT(0);
		}
	}

	int h;
	fprintf(fp, "#======================\n#init_bias\n");
	SAVE_IH_WEIGHTS(mp->init_bias);
	fprintf(fp, "#======================\n#hid_bias\n");
	SAVE_IH_WEIGHTS(mp->hid_bias);
	fprintf(fp, "#======================\n#hid_bias_srl\n");
	SAVE_IH_WEIGHTS(mp->hid_bias_srl);

	int a;
	fprintf(fp, "#======================\n#prev_act_bias\n");
	for (a = 0; a < ACTION_NUM; a++) {
		SAVE_IH_WEIGHTS(mp->prev_act_bias[a]);
	}

	fprintf(fp, "#======================\n#prev_deprel_bias\n");
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < mp->deprel_num; d++) {
			SAVE_IH_WEIGHTS(mp->prev_deprel_bias[a][d]);
		}
	}
	fprintf(fp, "#======================\n#prev_arg_bias\n");
	int b;
	int max = 1;
	for (b = 0; b < BANK_NUM; b++) {
		if (mp->role_num[b] > max) {
			max = mp->role_num[b];
		}
	}
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < max; d++) {
			SAVE_IH_WEIGHTS(mp->prev_arg_bias[a][d]);
		}
	}

	//save HID-HID weight matrixes
	int h1, h2;
	fprintf(fp, "#======================\n#hh_links\n#======================\n");
	for (l = 0; l < mp->hh_links_num; l++) {
		fprintf(fp, "#Link  %d (HH)\n", l);
		for (h1 = 0; h1 < ap->hid_size; h1++) {
			fprintf(fp, "%.20f", mp->hh_link[l].w[h1][0]);
			for (h2 = 1; h2 < ap->hid_size; h2++) {
				fprintf(fp, " %.20f", mp->hh_link[l].w[h1][h2]);
			}
			fprintf(fp, "\n");
		}
	}


	//save output weights
#define SAVE_OUT_LINK(out_link)\
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
		SAVE_OUT_LINK(mp->out_link_act);
	}

	fprintf(fp,"#out_link_la_label/ra_label\n");
	for (t = 0; t < mp->deprel_num; t++) {
		SAVE_OUT_LINK(mp->out_link_la_label);
		SAVE_OUT_LINK(mp->out_link_ra_label);
	}

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	int pos_num = ap->input_offset + mp->pos_info_out.num;
	fprintf(fp,"#out_link_pos\n");
	for (t = 0; t < pos_num; t++) {
		SAVE_OUT_LINK(mp->out_link_pos);
	}

	fprintf(fp,"#out_link_feat\n");
	int p;
	for (p = 0; p < pos_num; p++) {
		for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
			SAVE_OUT_LINK(mp->out_link_feat[p]);
		}
	}

	fprintf(fp,"#out_link_word\n");
	int f;
	for (p = 0; p < pos_num; p++) {
		for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
			for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
				SAVE_OUT_LINK(mp->out_link_word[p][f]);
			}
		}
	}

	fprintf(fp, "#out_link_sem_la_label\n");
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			SAVE_OUT_LINK(mp->out_link_sem_la_label[b]);
		}
	}
	fprintf(fp, "#out_link_sem_ra_label\n");
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			SAVE_OUT_LINK(mp->out_link_sem_ra_label[b]);
		}
	}

	fprintf(fp, "#out_link_sense\n");
	for (b = 0; b < BANK_NUM; b++) {
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			for (t = 0; t < mp->sense_num[b][l]; t++) {
				SAVE_OUT_LINK(mp->out_link_sense[b][l][0]);
			}
		}
	}


	fclose(fp);
}
//saves parameters
void save_weight_dels(APPROX_PARAMS *ap, MODEL_PARAMS *mp, char* fname) {

	FILE *fp;
	if ((fp = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}


#define SAVE_IH_WEIGHT_DELS(ih)\
	{\
	fprintf(fp, "%.20f", (ih).b_del[0]); \
	for (h = 1; h < ap->hid_size; h++) { \
		fprintf(fp, " %.20f", (ih).b_del[h]); \
	}\
	fprintf(fp, "\n");\
	}

	//save input links
	int l;
	fprintf(fp, "#ih_links\n#======================\n");
	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			int i, h;
			//+ 1  to represent the fact that there is
			//no word complying to the specification l
			fprintf(fp, "#Link %d, CPOS_TYPE\n", l);
			for (i = 0; i < mp->cpos_num + 1; i++) {
				SAVE_IH_WEIGHT_DELS(mp->ih_link_cpos[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, DEPREL_TYPE\n", l);
			// +2 : for ROOT relationship and to represent fact that there is
			// not word which complies with the specification l
			for (i = 0; i < mp->deprel_num + 2; i++) {
				SAVE_IH_WEIGHT_DELS(mp->ih_link_deprel[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, POS_TYPE\n", l);
			for (i = 0; i < mp->pos_info_in.num + 1; i++) {
				SAVE_IH_WEIGHT_DELS(mp->ih_link_pos[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, LEMMA_TYPE\n", l);
			for (i = 0; i < mp->lemma_num + 1; i++) {
				SAVE_IH_WEIGHT_DELS(mp->ih_link_lemma[l][i]);
			}
		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int p, f, h;
				fprintf(fp, "#Link %d, FEAT_TYPE (COMPOSED)\n", l);
				for (p = 0; p < mp->pos_info_in.num; p++) {
					for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
						SAVE_IH_WEIGHT_DELS(mp->ih_link_feat[l][p][f]);
					}
				}
				//link for no word
				SAVE_IH_WEIGHT_DELS(mp->ih_link_feat[l][mp->pos_info_in.num][0]);
			}
			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				int i,h;
				fprintf(fp, "#Link %d, FEAT_TYPE (ELEMENTARY)\n", l);
				for (i = 0; i < mp->elfeat_num + 1; i++) {
					SAVE_IH_WEIGHT_DELS(mp->ih_link_elfeat[l][i]);
				}
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			fprintf(fp, "#Link %d, SENSE_TYPE\n", l);
			int b, lemma, s, h;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					for (s = 0; s < mp->sense_num[b][lemma]; s++) {
						SAVE_IH_WEIGHT_DELS(mp->ih_link_sense[l][b][lemma][s]);
					}
				}
			}
			//link for no word and no sense
			SAVE_IH_WEIGHT_DELS(mp->ih_link_sense[l][BANK_NUM][0][0]);
			SAVE_IH_WEIGHT_DELS(mp->ih_link_sense[l][BANK_NUM + 1][0][0]);
		} else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
			fprintf(fp, "#Link %d, WORD_TYPE\n", l);
			int p, f, w, h;
			for (p = 0; p < mp->pos_info_in.num; p++) {
				for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
					for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
						SAVE_IH_WEIGHT_DELS(mp->ih_link_word[l][p][f][w]);
					}
				}
			}
			//link for no word
			SAVE_IH_WEIGHT_DELS(mp->ih_link_word[l][mp->pos_info_in.num][0][0]);
		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			int i,h;
			fprintf(fp, "#Link %d, SYNT_LAB_PATH_TYPE\n", l);
			int path_num = get_synt_path_feature_size(mp);
			for (i = 0; i < path_num + 1; i++) {
				SAVE_IH_WEIGHT_DELS(mp->ih_link_synt_lab_path[l][i]);
			}
		} else {
			ASSERT(0);
		}
	}

	int h;
	fprintf(fp, "#======================\n#init_bias\n");
	SAVE_IH_WEIGHT_DELS(mp->init_bias);
	fprintf(fp, "#======================\n#hid_bias\n");
	SAVE_IH_WEIGHT_DELS(mp->hid_bias);
	fprintf(fp, "#======================\n#hid_bias_srl\n");
	SAVE_IH_WEIGHT_DELS(mp->hid_bias_srl);

	int a;
	fprintf(fp, "#======================\n#prev_act_bias\n");
	for (a = 0; a < ACTION_NUM; a++) {
		SAVE_IH_WEIGHT_DELS(mp->prev_act_bias[a]);
	}

	fprintf(fp, "#======================\n#prev_deprel_bias\n");
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < mp->deprel_num; d++) {
			SAVE_IH_WEIGHT_DELS(mp->prev_deprel_bias[a][d]);
		}
	}
	fprintf(fp, "#======================\n#prev_arg_bias\n");
	int b;
	int max = 1;
	for (b = 0; b < BANK_NUM; b++) {
		if (mp->role_num[b] > max) {
			max = mp->role_num[b];
		}
	}
	for (a = 0; a < ACTION_NUM; a++) {
		int d;
		for (d = 0; d < max; d++) {
			SAVE_IH_WEIGHT_DELS(mp->prev_arg_bias[a][d]);
		}
	}


	//save HID-HID weight matrixes
	int h1, h2;
	fprintf(fp, "#======================\n#hh_links\n#======================\n");
	for (l = 0; l < mp->hh_links_num; l++) {
		fprintf(fp, "#Link  %d (HH)\n", l);
		for (h1 = 0; h1 < ap->hid_size; h1++) {
			fprintf(fp, "%.20f", mp->hh_link[l].w_del[h1][0]);
			for (h2 = 1; h2 < ap->hid_size; h2++) {
				fprintf(fp, " %.20f", mp->hh_link[l].w_del[h1][h2]);
			}
			fprintf(fp, "\n");
		}
	}


	//save output weights
#define SAVE_OUT_LINK_DEL(out_link)\
	{\
	fprintf(fp, "%.20f", (out_link).b_del[t]);\
	for (i = 0; i < ap->hid_size; i++) { \
		fprintf(fp, " %.20f", (out_link).w_del[t][i]);\
	}\
	fprintf(fp, "\n");\
	}
	fprintf(fp, "#======================\n#out_links\n#======================\n");

	int t, i;
	fprintf(fp,"#out_link_act\n");
	for (t = 0; t < ACTION_NUM; t++) {
		SAVE_OUT_LINK_DEL(mp->out_link_act);
	}

	fprintf(fp,"#out_link_la_label/ra_label\n");
	for (t = 0; t < mp->deprel_num; t++) {
		SAVE_OUT_LINK_DEL(mp->out_link_la_label);
		SAVE_OUT_LINK_DEL(mp->out_link_ra_label);
	}

	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	int pos_num = ap->input_offset + mp->pos_info_out.num;
	fprintf(fp,"#out_link_pos\n");
	for (t = 0; t < pos_num; t++) {
		SAVE_OUT_LINK_DEL(mp->out_link_pos);
	}

	fprintf(fp,"#out_link_feat\n");
	int p;
	for (p = 0; p < pos_num; p++) {
		for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
			SAVE_OUT_LINK_DEL(mp->out_link_feat[p]);
		}
	}

	fprintf(fp,"#out_link_word\n");
	int f;
	for (p = 0; p < pos_num; p++) {
		for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
			for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
				SAVE_OUT_LINK_DEL(mp->out_link_word[p][f]);
			}
		}
	}

	fprintf(fp, "#out_link_sem_la_label\n");
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			SAVE_OUT_LINK_DEL(mp->out_link_sem_la_label[b]);
		}
	}
	fprintf(fp, "#out_link_sem_ra_label\n");
	for (b = 0; b < BANK_NUM; b++) {
		for (t = 0; t < mp->role_num[b]; t++) {
			SAVE_OUT_LINK_DEL(mp->out_link_sem_ra_label[b]);
		}
	}

	fprintf(fp, "#out_link_sense\n");
	for (b = 0; b < BANK_NUM; b++) {
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			for (t  = 0; t <  mp->sense_num[b][l]; t++) {
				SAVE_OUT_LINK_DEL(mp->out_link_sense[b][l][0]);
			}
		}
	}


	fclose(fp);
}

//print sentence
char *print_sent(SENTENCE *sent, int with_links) {
	ASSERT(sent->len >= 0 && sent->len <= MAX_SENT_LEN);
	DEF_ALLOC_ARRAY(s, char, sent->len * MAX_LINE + 1);
	s[0] = 0;
	char buf[MAX_LINE];
	int i;

	for (i = 1; i <= sent->len; i++) {
		sprintf(buf, "%d\t%s\t%d\t%d\t%s\t%d\t%s\t%d\t%s\t%d\t%s\t%d\t%d\t",
				i,
				sent->s_word[i], sent->word_in[i], sent->word_out[i],
				sent->s_lemma[i], sent->lemma[i],
				sent->s_cpos[i], sent->cpos[i],
				sent->s_pos[i], sent->pos[i],
				sent->s_feat[i], sent->feat_in[i], sent->feat_out[i]);
		strcat(s, buf);

		if (sent->elfeat_len[i] == 0) {
			strcat(s, "_");
		} else {
			int f;
			for (f = 0; f < sent->elfeat_len[i]; f++) {
				if (f != 0) {
					strcat(s, "|");
				}
				sprintf(buf, "%d", sent->elfeat[i][f]);
				strcat(s, buf);
			}
		}
		sprintf(buf, "\t%d", sent->bank[i]);
		strcat(s, buf);

		if (with_links) {
			sprintf(buf, "\t%d\t%s\t%d\t%s\t%d",
					sent->head[i],
					sent->s_deprel[i], sent->deprel[i], sent->s_sense[i], sent->sense[i]);
			strcat(s, buf);
			int a;
			for (a = 0; a < sent->arg_num[i]; a++) {
				sprintf(buf, "\t%d\t%s\t%d", sent->args[i][a], sent->s_arg_roles[i][a],
						sent->arg_roles[i][a]);
				strcat(s, buf);
			}

		}
		strcat(s,"\n");
	}
	return s;
}

void append_sentence(char *fname, SENTENCE *sen, int with_links) {
	FILE *fp;
	if ((fp = fopen(fname, "w+")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}
	char *s = print_sent(sen, with_links);
	fprintf(fp, "%s\n", s);
	free(s);
}

void save_sentence(FILE *fp, SENTENCE *sen, int with_links) {
	char *s = print_sent(sen, with_links);
	fprintf(fp, "%s\n", s);
	free(s);
}

SENTENCE *create_blind_sent(SENTENCE *gld_sent) {
	DEF_ALLOC(blind_sent, SENTENCE);
	memcpy(blind_sent, gld_sent, sizeof(SENTENCE));
	int i;
	for (i = 1; i <= blind_sent->len; i++) {
		blind_sent->head[i] = -1;
		strcpy(blind_sent->s_deprel[i], "");
		blind_sent->deprel[i] = ROOT_DEPREL;
	}

	return blind_sent;
}


void fill_decomp_feats(MODEL_PARAMS *mp, SENTENCE *sen, int t, char *decomp_feats) {
	sen->elfeat_len[t] = 0;
	if (strcmp(decomp_feats, "_") == 0) {
		return;
	}

	char *s = strtok(decomp_feats, "|");
	if (s == NULL) {
		return;
	} else {
		sen->elfeat[t][sen->elfeat_len[t]++] = atoi(s);
	}
	while ((s = strtok(NULL, "|")) != NULL) {
		if (sen->elfeat_len[t] >= MAX_SIMULT_FEAT) {
			fprintf(stderr, "Error: number of features for sentence %d is larger than MAX_SIMULT_FEAT (%d)\n", t, MAX_SIMULT_FEAT);
			exit(1);
		}
		sen->elfeat[t][sen->elfeat_len[t]++] = atoi(s);
		if (sen->elfeat[t][sen->elfeat_len[t]-1] >= mp->elfeat_num) {
			fprintf(stderr, "Error: wrong feature id %d for sentence %d, it should not exceed %d\n", sen->elfeat[t][sen->elfeat_len[t]-1], t, mp->elfeat_num - 1);
		}
	}
}

int is_blind_file(char *fname) {
	DEF_FOPEN(fp, fname, "r");

	char buffer[MAX_LINE];

	while (fgets(buffer, MAX_LINE, fp) != NULL) {
		if (buffer[0] == '#') {
			continue;
		}
		char *s = strtok(buffer, FIELD_SEPS);
		if (s == NULL) {
			break;
		}
		//skip 14 fields
		int f;
		for (f = 1; f < 15; f++) {
			strtok(NULL, FIELD_SEPS);
		}
		//this field is head
		fclose(fp);
		return (strtok(NULL, FIELD_SEPS) == NULL);
	}

	fprintf(stderr, "Empty testing file. Stopped.\n");
	fclose(fp);
	exit(1);

}

int* remove_doubles(int a[], int len, int* new_len) {
	if (len == 0) {
		*new_len = 0;
		return a;
	}
	DEF_ALLOC_ARRAY(b, int,  sizeof(int) * len);
	int i;
	*new_len = 0;
	for (i = 0; i < len; i++) {
		if (i == 0 || a[i - 1] != a[i]) {
			b[(*new_len)++] = a[i];
		}
	}
	return b;
}

// if a > b return positive, a == b - return 0, else return negative
int lexicograph_ignore_double_entries_comp(int a[], int a_len, int b[], int b_len) {
	if (a_len == 0) {
		if (b_len == 0) {
			return 0;
		} else {
			return -1;
		}
	} else if (b_len == 0) {
		return 1;
	}
	int a_no_dup_len;
	int *a_no_dup = remove_doubles(a, a_len, &a_no_dup_len);
	int b_no_dup_len;
	int *b_no_dup = remove_doubles(b, b_len, &b_no_dup_len);

	int i;
	int min_len = a_no_dup_len < b_no_dup_len ? a_no_dup_len : b_no_dup_len;
	for (i = 0; i < min_len; i++) {
		if (a_no_dup[i] < b_no_dup[i]) {
			free(a_no_dup); free(b_no_dup);
			return -1;
		} else if (a_no_dup[i] > b_no_dup[i]) {
			free(a_no_dup); free(b_no_dup);
			return 1;
		}
	}
	free(a_no_dup); free(b_no_dup);
	if (a_no_dup_len < b_no_dup_len) {
		return -1;
	} else if (a_no_dup_len > b_no_dup_len) {
		return 1;
	} else {
		return 0;
	}
}

void sort(int a[], int n) {
	//bubble sort
	int i, j;
	for (i = 0; i < n - 1; i++) {
		for (j = 0; j < n - 1 - i; j++) {
			if (a[j+1] < a[j]) {
				int tmp = a[j];
				a[j] = a[j + 1];
				a[j + 1] = tmp;
			}
		}
	}
}

void set_right_indices(SENTENCE *sen) {
	//first syntax
	int w;
	for (w = 1; w <= sen->len; w++) {
		if (sen->head[w] > 0) {
			int head = sen->head[w];
			if (head > w) {
				sen->synt_right_indices[w][sen->synt_right_degree[w]++] = head;
			} else {
				//loops are not allowed
				ASSERT(head != w);
				sen->synt_right_indices[head][sen->synt_right_degree[head]++] = w;
			}
		}

	}

	// now srl
	for (w = 1; w <= sen->len; w++) {
		//take arguments
		int a, arg;
		for (a = 0; a < sen->arg_num[w]; a++) {
			arg = sen->args[w][a];
			if (arg > w) {
				sen->srl_right_indices[w][sen->srl_right_degree[w]++] = arg;
			} else {
				// loops are not supported
				ASSERT(arg != w);
				sen->srl_right_indices[arg][sen->srl_right_degree[arg]++] = w;
			}
		}
	}
	for (w = 1; w <= sen->len; w++) {
		sort(sen->srl_right_indices[w], sen->srl_right_degree[w]);
		sort(sen->synt_right_indices[w], sen->synt_right_degree[w]);
	}

}


SENTENCE *read_sentence(MODEL_PARAMS *mp, FILE *fp, int with_links) {
	DEF_ALLOC(sen, SENTENCE);

	char buffer[MAX_LINE];

	int t = 1;
	int read_smth = 0;
	while (fgets(buffer, MAX_LINE, fp) != NULL) {
		if (buffer[0] == '#') {
			read_smth = 1;
			continue;
		}
		char *s = strtok(buffer, FIELD_SEPS);
		if (s == NULL) {
			break;
		}
		strncpy(sen->s_word[t], strtok(NULL, FIELD_SEPS), MAX_REC_LEN);
		sen->word_in[t] = atoi(strtok(NULL, FIELD_SEPS));
		sen->word_out[t] = atoi(strtok(NULL, FIELD_SEPS));

		strncpy(sen->s_lemma[t], strtok(NULL, FIELD_SEPS), MAX_REC_LEN);
		sen->lemma[t] = atoi(strtok(NULL, FIELD_SEPS));

		strncpy(sen->s_cpos[t], strtok(NULL, FIELD_SEPS), MAX_REC_LEN);
		sen->cpos[t] = atoi(strtok(NULL, FIELD_SEPS));

		strncpy(sen->s_pos[t], strtok(NULL, FIELD_SEPS), MAX_REC_LEN);
		sen->pos[t] = atoi(strtok(NULL, FIELD_SEPS));

		strncpy(sen->s_feat[t], strtok(NULL, FIELD_SEPS), MAX_REC_LEN);
		sen->feat_in[t] = atoi(strtok(NULL, FIELD_SEPS));
		sen->feat_out[t] = atoi(strtok(NULL, FIELD_SEPS));


		char *decomp_feats = strtok(NULL, FIELD_SEPS);

		sen->bank[t] = atoi(strtok(NULL, FIELD_SEPS));

#define CHECK_RANGE(x, size, name) if ((x) >= (size)) { fprintf(stderr, "Error: wrong %s: %d, exceeds allowed values of %d\n", name, (x), (size - 1)); exit(1);}
		CHECK_RANGE(sen->pos[t], mp->pos_info_in.num, "POS");
		CHECK_RANGE(sen->cpos[t], mp->cpos_num, "CPOS");
		CHECK_RANGE(sen->lemma[t], mp->lemma_num, "LEMMA");
		CHECK_RANGE(sen->feat_in[t], mp->pos_info_in.feat_infos[sen->pos[t]].num, "FEAT_IN");
		CHECK_RANGE(sen->feat_out[t], mp->pos_info_out.feat_infos[sen->pos[t]].num, "FEAT_OUT");
		if (sen->word_in[t] >= mp->pos_info_in.feat_infos[sen->pos[t]].word_infos[sen->feat_in[t]].num) {
#if PROGRESSDOT
			printf(".");
#endif
		}
		CHECK_RANGE(sen->word_in[t],  mp->pos_info_in.feat_infos[sen->pos[t]].word_infos[sen->feat_in[t]].num, "WORD_IN");

		CHECK_RANGE(sen->word_out[t], mp->pos_info_out.feat_infos[sen->pos[t]].word_infos[sen->feat_out[t]].num, "WORD_OUT");


		if (with_links) {
			sen->head[t] = atoi(strtok(NULL, FIELD_SEPS));
			char *s_deprel = strtok(NULL, FIELD_SEPS);
			strncpy(sen->s_deprel[t], s_deprel, MAX_REC_LEN);
			sen->deprel[t] = atoi(strtok(NULL, FIELD_SEPS));

			CHECK_RANGE(sen->deprel[t], mp->deprel_num, "DEPREL");

			//read FILLPRED column
			if(USE_FILLPRED){
				char *fillpred = strtok(NULL, FIELD_SEPS);
				if(strcmp(fillpred,"_")==0){
					sen->fill_pred[t]=0;
				}
				else if(strcmp(fillpred,"Y")==0){
					sen->fill_pred[t]=1;
				}
				else{
					fprintf(stderr, "Error: wrong value in FILLPRED column: %s\n", fillpred); exit(1);
				}
			}

			char *s_sense = strtok(NULL, FIELD_SEPS);
			strncpy(sen->s_sense[t], s_sense, MAX_REC_LEN);
			sen->sense[t] = atoi(strtok(NULL, FIELD_SEPS));
			if (sen->bank[t] > 0) {
				CHECK_RANGE(sen->sense[t], mp->sense_num[sen->bank[t]][sen->lemma[t]], "SENSE");
			}

			char *child_token;
			sen->arg_num[t] = 0;
			while ( (child_token = strtok(NULL, FIELD_SEPS))!= NULL) {
				if (sen->arg_num[t] >= MAX_ARG_NUM) {
					fprintf(stderr, "Error: to many arguments: exceeds MAX_ARG_NUM = %d, increase MAX_ARG_NUM and rebuild\n", MAX_ARG_NUM); exit(1);
				}

				sen->args[t][sen->arg_num[t]] = atoi(child_token);
				char *s_role = strtok(NULL, FIELD_SEPS);
				strncpy(sen->s_arg_roles[t][sen->arg_num[t]], s_role, MAX_REC_LEN);
				sen->arg_roles[t][sen->arg_num[t]] = atoi(strtok(NULL, FIELD_SEPS));
				sen->arg_num[t]++;
			}

		}

		fill_decomp_feats(mp, sen, t, decomp_feats);

		t++;
	}


	//starting symbols
	strcpy(sen->s_word[0], "^^^");
	sen->word_in[0] = 0;
	sen->word_out[0] = 0;

	strcpy(sen->s_lemma[0], "^^^");
	sen->lemma[0] = mp->lemma_num;

	strcpy(sen->s_cpos[0], "^^^");
	sen->cpos[0] = mp->cpos_num;

	strcpy(sen->s_pos[0], "^^^");
	sen->pos[0] = mp->pos_info_in.num;
	ASSERT(mp->pos_info_in.num == mp->pos_info_out.num);

	strcpy(sen->s_feat[0], "^^^");
	sen->feat_in[0] =  0;
	sen->feat_out[0] = 0;
	sen->elfeat_len[0] = 1;
	sen->elfeat[0][0] = mp->elfeat_num;

	sen->bank[0] = -1;

	if (with_links) {
		sen->head[0] = 0;
		strcpy(sen->s_deprel[0], "^^^");
		sen->deprel[0] = mp->deprel_num;

		sen->fill_pred[0]=0;

		strncpy(sen->s_sense[0], "_", MAX_REC_LEN);
		sen->sense[0] = -1;
		sen->arg_num[0] = 0;
	}

	//ending symbol
	strcpy(sen->s_word[t], "$$$");
	sen->word_in[t] = 0;
	sen->word_out[t] = 0;

	strcpy(sen->s_lemma[t], "$$$");
	sen->lemma[t] = mp->lemma_num;

	strcpy(sen->s_cpos[t], "$$$");
	sen->cpos[t] = mp->cpos_num;

	strcpy(sen->s_pos[t], "$$$");
	sen->pos[t] = mp->pos_info_in.num;

	strcpy(sen->s_feat[t], "$$$");
	sen->feat_in[t] =  0;
	sen->feat_out[t] = 0;
	sen->elfeat_len[t] = 1;
	sen->elfeat[t][0] = mp->elfeat_num;

	sen->bank[t] = -1;

	if (with_links) {
		sen->head[t] = 0;
		strcpy(sen->s_deprel[t], "$$$");
		sen->deprel[t] = mp->deprel_num;

		strncpy(sen->s_sense[t], "_", MAX_REC_LEN);
		sen->sense[t] = -1;
		sen->arg_num[t] = 0;
	}

	sen->len = t - 1;
	if (sen->len == 0 && !read_smth) {
		free(sen);
		return NULL;
	}  else {

		if (with_links) {
			set_right_indices(sen);
		}

		return sen;
	}

}

void cat_print_i(char *s, char *field, int x) {
	char buf[MAX_LINE];
	sprintf(buf, "%s %d\n", field, x);
	ASSERT(strlen(buf) + strlen(s) + 1 < MAX_LINE * 25);
	strcat(s, buf);
}

void cat_print_f(char *s, char *field, double x) {
	char buf[MAX_LINE];
	sprintf(buf, "%s %e\n", field, x);
	ASSERT(strlen(buf) + strlen(s) + 1 < MAX_LINE * 25);
	strcat(s, buf);
}

void cat_print_s(char *s, char *field, char *x) {
	char buf[MAX_LINE];
	sprintf(buf,"%s %s\n", field, x);
	ASSERT(strlen(buf) + strlen(s) + 1 < MAX_LINE * 25);
	strcat(s, buf);
}

char *get_params_string(APPROX_PARAMS *ap) {
	DEF_ALLOC_ARRAY(s, char,  sizeof(char) * MAX_LINE * 25);

	if (ap->approx_type != 0) {
		strcpy(s, "APPROX_TYPE");
		if (ap->approx_type == FF) {
			strcat(s, " FF\n");
		} else {
			ASSERT(ap->approx_type == MF);
			strcat(s, " MF\n");
		}
		strcat(s, "\n");
	}

	cat_print_s(s,"MODEL_NAME", ap->model_name);
	cat_print_s(s,"TRAIN_FILE", ap->train_file);
	cat_print_i(s,"TRAIN_DATASET_SIZE", ap->train_dataset_size);
	cat_print_s(s,"TEST_FILE", ap->test_file);
	cat_print_i(s,"TEST_DATASET_SIZE", ap->test_dataset_size);
	cat_print_s(s,"OUT_FILE", ap->out_file);
	cat_print_s(s,"IO_SPEC_FILE", ap->io_spec_file);
	cat_print_i(s, "INP_FEAT_MODE", ap->inp_feat_mode);
	cat_print_s(s,"IH_LINK_SPEC_FILE",ap->ih_links_spec_file);
	cat_print_s(s, "HH_LINK_SPEC_FILE",ap->hh_links_spec_file);
	cat_print_i(s, "INPUT_OFFSET", ap->input_offset);
	cat_print_i(s,"PARSING_MODE", ap->parsing_mode);
	cat_print_i(s, "SRL_EARLY_REDUCE", ap->is_srl_early_reduce);
	cat_print_i(s, "SYNT_EARLY_REDUCE", ap->is_synt_early_reduce);
	cat_print_i(s, "SRL_DEPROJECTIVIZATION", ap->intern_srl_deproj);
	cat_print_i(s, "SYNT_DEPROJECTIVIZATION", ap->intern_synt_deproj);

	strcat(s, "\n");

	cat_print_i(s, "BEAM", ap->beam);
	cat_print_i(s, "SEARCH_BR_FACTOR", ap->search_br_factor);
	cat_print_i(s, "RETURN_CAND", ap->return_cand);
	cat_print_i(s, "CAND_NUM", ap->cand_num);

	cat_print_i(s, "SEED", ap->seed);
	cat_print_f(s, "RAND_RANGE", ap->rand_range);
	cat_print_f(s, "EM_RAND_RANGE", ap->em_rand_range);
	strcat(s, "\n");


	cat_print_i(s, "HID_SIZE", ap->hid_size);
	strcat(s, "\n");

	cat_print_f(s,"INIT_ETA", ap->init_eta);
	cat_print_f(s,"ETA_RED_RATE", ap->eta_red_rate);
	cat_print_f(s,"MAX_ETA_RED", ap->max_eta_red);
	strcat(s, "\n");

	cat_print_f(s,"INIT_REG", ap->init_reg);
	cat_print_f(s,"REG_RED_RATE", ap->reg_red_rate);
	cat_print_f(s,"MAX_REG_RED", ap->max_reg_red);
	strcat(s, "\n");

	cat_print_f(s,"MOM", ap->mom);
	strcat(s, "\n");

	cat_print_i(s,"MAX_LOOPS", ap->max_loops);
	cat_print_i(s,"MAX_LOOPS_WO_ACCUR_IMPR", ap->max_loops_wo_accur_impr);
	cat_print_i(s,"LOOPS_BETWEEN_VAL", ap->loops_between_val);
	strcat(s, "\n");

	cat_print_i(s,"DISTINGUISH_SRL_AND_SYNT_BIAS", ap->disting_biases);

	// TODO restore when DD and MBR part is released
	//cat_print_f(s,"MBR_COEFF", ap->mbr_coeff);

	//cat_print_i(s, "DD_KERNEL_TYPE", ap->dd_kernel_type);
	//cat_print_i(s,"USE_RERANK_DD_COMP", ap->use_dd_comp);
	//cat_print_i(s, "USE_RERANK_EXPL_COMP", ap->use_expl_comp);
	//cat_print_f(s,"RERANK_LPROB_W", ap->lprob_w);
	//cat_print_f(s,"RERANK_DD_W", ap->dd_w);

	strcat(s, "\n");


	return s;

}

#define IF_EQ_SET_N_CONT_S(name, var) \
	if (strcmp(field, name) == 0) {\
		char *s = field_missing_wrapper(strtok(NULL, FIELD_SEPS), name);\
		ASSERT(strlen(s) < MAX_LINE); \
		strcpy(var, s);\
		continue; \
	}
#define IF_EQ_SET_N_CONT_I(name, var) \
	if (strcmp(field, name) == 0) {\
		var =  atoi(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name));\
		continue; \
	}
#define IF_EQ_SET_N_CONT_F(name, var) \
	if (strcmp(field, name) == 0) {\
		var =  atof(field_missing_wrapper(strtok(NULL, FIELD_SEPS), name));\
		continue; \
	}

APPROX_PARAMS *read_params(char *fname) {
	DEF_FOPEN(fp, fname, "r");
	DEF_ALLOC(ap, APPROX_PARAMS);

	char buffer[MAX_LINE], *s;
	int line = 0;
	do {
		do {
			s = fgets(buffer, MAX_LINE, fp);
			if (s == NULL) {
				break;
			}
			line++;
		} while (buffer[0] == '#' || buffer[0] == '\0');
		if (s == NULL) {
			break;
		}

		char *field = strtok(buffer, FIELD_SEPS);
		if (field == NULL) {
			//empty line
			continue;
		}

		if (strcmp(field, "APPROX_TYPE") == 0) {
			char *type = field_missing_wrapper(strtok(NULL, FIELD_SEPS), "APPROX_TYPE");
			if (strcmp(type, "FF") == 0) {
				ap->approx_type = FF;
			} else if (strcmp(type, "MF") == 0) {
				ap->approx_type = MF;
			} else {
				fprintf(stderr, "Wrong value '%s' for 'APPROX_TYPE' field\n", type);
				exit(1);
			}
			continue;
		}

		IF_EQ_SET_N_CONT_S("MODEL_NAME", ap->model_name);
		IF_EQ_SET_N_CONT_S("TRAIN_FILE", ap->train_file);
		IF_EQ_SET_N_CONT_I("TRAIN_DATASET_SIZE", ap->train_dataset_size);
		IF_EQ_SET_N_CONT_S("TEST_FILE", ap->test_file);
		IF_EQ_SET_N_CONT_I("TEST_DATASET_SIZE", ap->test_dataset_size);
		IF_EQ_SET_N_CONT_S("OUT_FILE", ap->out_file);
		IF_EQ_SET_N_CONT_S("IO_SPEC_FILE", ap->io_spec_file);
		IF_EQ_SET_N_CONT_I("INP_FEAT_MODE", ap->inp_feat_mode);
		IF_EQ_SET_N_CONT_S("IH_LINK_SPEC_FILE",ap->ih_links_spec_file);
		IF_EQ_SET_N_CONT_S("HH_LINK_SPEC_FILE",ap->hh_links_spec_file);
		IF_EQ_SET_N_CONT_I("INPUT_OFFSET", ap->input_offset);
		IF_EQ_SET_N_CONT_I("PARSING_MODE", ap->parsing_mode);
		IF_EQ_SET_N_CONT_I("SRL_EARLY_REDUCE", ap->is_srl_early_reduce);
		IF_EQ_SET_N_CONT_I("SYNT_EARLY_REDUCE", ap->is_synt_early_reduce);
		IF_EQ_SET_N_CONT_I("SRL_DEPROJECTIVIZATION", ap->intern_srl_deproj);
		IF_EQ_SET_N_CONT_I("SYNT_DEPROJECTIVIZATION", ap->intern_synt_deproj);

		IF_EQ_SET_N_CONT_I("BEAM", ap->beam);
		IF_EQ_SET_N_CONT_I("SEARCH_BR_FACTOR", ap->search_br_factor);
		IF_EQ_SET_N_CONT_I("RETURN_CAND", ap->return_cand);
		IF_EQ_SET_N_CONT_I("CAND_NUM", ap->cand_num);


		IF_EQ_SET_N_CONT_I("SEED", ap->seed);
		IF_EQ_SET_N_CONT_F("RAND_RANGE", ap->rand_range);
		IF_EQ_SET_N_CONT_F("EM_RAND_RANGE", ap->em_rand_range);

		IF_EQ_SET_N_CONT_I("HID_SIZE", ap->hid_size);


		IF_EQ_SET_N_CONT_F("INIT_ETA", ap->init_eta);
		IF_EQ_SET_N_CONT_F("ETA_RED_RATE", ap->eta_red_rate);
		IF_EQ_SET_N_CONT_F("MAX_ETA_RED", ap->max_eta_red);

		IF_EQ_SET_N_CONT_F("INIT_REG", ap->init_reg);
		IF_EQ_SET_N_CONT_F("REG_RED_RATE", ap->reg_red_rate);
		IF_EQ_SET_N_CONT_F("MAX_REG_RED", ap->max_reg_red);

		IF_EQ_SET_N_CONT_F("MOM", ap->mom);

		IF_EQ_SET_N_CONT_I("MAX_LOOPS", ap->max_loops);
		IF_EQ_SET_N_CONT_I("MAX_LOOPS_WO_ACCUR_IMPR", ap->max_loops_wo_accur_impr);
		IF_EQ_SET_N_CONT_I("LOOPS_BETWEEN_VAL", ap->loops_between_val);
		IF_EQ_SET_N_CONT_I("DISTINGUISH_SRL_AND_SYNT_BIAS", ap->disting_biases);

		IF_EQ_SET_N_CONT_F("MBR_COEFF", ap->mbr_coeff);

		IF_EQ_SET_N_CONT_I("DD_KERNEL_TYPE", ap->dd_kernel_type);
		IF_EQ_SET_N_CONT_I("USE_RERANK_DD_COMP", ap->use_dd_comp);
		IF_EQ_SET_N_CONT_I("USE_RERANK_EXPL_COMP", ap->use_expl_comp);
		IF_EQ_SET_N_CONT_F("RERANK_LPROB_W", ap->lprob_w);
		IF_EQ_SET_N_CONT_F("RERANK_DD_W", ap->dd_w);



		fprintf(stderr, "Error: unknown field '%s' in the settings file\n", field);
		exit(1);
	} while (1);
	fclose(fp);
	return ap;
}

//an input feature specification, these features are used as  inputs to hidden layer
//specification approach is replicated from MALT parser
int opt_int_field_wrapper(char* s) {
	if (s == NULL) {
		return 0;
	} else {
		return atoi(s);
	}
}

void skip_and_read(char *buffer, char *fname, FILE *fp) {
	fgets(buffer, MAX_LINE, fp);
	while (buffer[0] == '#' || buffer[0] == '\0' || buffer[0] == '\n') {
		if (fgets(buffer, MAX_LINE, fp) == NULL) {
			break;
		}
	}

	if (strcmp(buffer,"") == 0) {
		fprintf(stderr, "Error: file %s is not complete\n", fname);
		exit(1);
	}
}

#define check_if_exceeds(val, s_val, bound, s_bound) if ( (val) > (bound) ) { fprintf(stderr, "Error: %s (%d) exceeds %s (%d)\n", s_val, val, s_bound, bound); exit(1); }

//reads IO specification file
void read_io_spec(char *fname, MODEL_PARAMS *mp) {
	FILE *fp;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}

	//CPOS
	char buffer[MAX_LINE];
	skip_and_read(buffer, fname, fp);
	mp->cpos_num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "cpos_num", fname));
	check_if_exceeds(mp->cpos_num, "cpos_num", MAX_CPOS_SIZE, "MAX_CPOS_SIZE");


	//POS
	skip_and_read(buffer, fname, fp);
	mp->pos_info_in.num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "pos_num", fname));
	mp->pos_info_out.num = mp->pos_info_in.num;
	if (mp->pos_info_in.num > MAX_POS_INP_SIZE || mp->pos_info_out.num > MAX_POS_OUT_SIZE) {
		fprintf(stderr, "Error: pos_info_in(out).num (%d) exceeds MAX_POS_OUT_SIZE (%d) or/and MAX_POS_INP_SIZE (%d)\n", mp->pos_info_out.num, MAX_POS_OUT_SIZE, MAX_POS_INP_SIZE);
		exit(1);
	}


	//FEAT IN
	skip_and_read(buffer, fname, fp);

	int pos_idx = 0;
	mp->pos_info_in.feat_infos[0].num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "feat[0]", fname));
	check_if_exceeds(mp->pos_info_in.feat_infos[pos_idx].num, "feat_num",  MAX_FEAT_INP_SIZE, "MAX_FEAT_INP_SIZE");
	for (pos_idx = 1; pos_idx < mp->pos_info_in.num; pos_idx++) {
		mp->pos_info_in.feat_infos[pos_idx].num = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "feat[pos_idx]", fname));
		check_if_exceeds(mp->pos_info_in.feat_infos[pos_idx].num, "feat_num",  MAX_FEAT_INP_SIZE, "MAX_FEAT_INP_SIZE");
	}

	//reserve for $$$ or ^^^ tag (not added in the real set) (it can be the same, because ^^^ - never happens in output, $$$ - never happens in input)
	mp->pos_info_in.feat_infos[mp->pos_info_in.num].num = 1;

	//WORD IN
	for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
		skip_and_read(buffer, fname, fp);
		int feat_idx = 0;
		mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS),
				"word[pos_idx][feat_idx]", fname));
		check_if_exceeds(mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num, "word_num", MAX_WORD_INP_SIZE, "MAX_WORD_INP_SIZE");

		for (feat_idx = 1; feat_idx < mp->pos_info_in.feat_infos[pos_idx].num; feat_idx++) {
			mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "word[pos_idx][feat_idx]", fname));
			check_if_exceeds(mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num, "word_num", MAX_WORD_INP_SIZE, "MAX_WORD_INP_SIZE");
		}
	}

	//^^^
	mp->pos_info_in.feat_infos[mp->pos_info_in.num].word_infos[0].num = 1;



	//FEAT OUT
	skip_and_read(buffer, fname, fp);
	pos_idx = 0;
	mp->pos_info_out.feat_infos[pos_idx].num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "feat[0]", fname));
	check_if_exceeds(mp->pos_info_out.feat_infos[pos_idx].num, "feat_num",  MAX_FEAT_OUT_SIZE, "MAX_FEAT_OUT_SIZE");
	for (pos_idx = 1; pos_idx < mp->pos_info_out.num; pos_idx++) {
		mp->pos_info_out.feat_infos[pos_idx].num = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "feat[pos_idx]", fname));
		check_if_exceeds(mp->pos_info_out.feat_infos[pos_idx].num, "feat_num",  MAX_FEAT_OUT_SIZE, "MAX_FEAT_OUT_SIZE");
	}

	//reserve for $$$ or ^^^ tag (not added in the real set) (it can be the same, because ^^^ - never happens in output, $$$ - never happens in input)
	mp->pos_info_out.feat_infos[mp->pos_info_out.num].num = 1;

	//WORD OUT
	for (pos_idx = 0; pos_idx < mp->pos_info_out.num; pos_idx++) {
		skip_and_read(buffer, fname, fp);
		int feat_idx = 0;
		mp->pos_info_out.feat_infos[pos_idx].word_infos[feat_idx].num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS),
				"word[pos_idx][feat_idx]", fname));
		check_if_exceeds(mp->pos_info_out.feat_infos[pos_idx].word_infos[feat_idx].num, "word_num",  MAX_WORD_OUT_SIZE, "MAX_WORD_OUT_SIZE");

		for (feat_idx = 1; feat_idx < mp->pos_info_out.feat_infos[pos_idx].num; feat_idx++) {
			mp->pos_info_out.feat_infos[pos_idx].word_infos[feat_idx].num = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "word[pos_idx][feat_idx]", fname));
			check_if_exceeds(mp->pos_info_out.feat_infos[pos_idx].word_infos[feat_idx].num, "word_num",  MAX_WORD_OUT_SIZE, "MAX_WORD_OUT_SIZE");
		}
	}

	// $$$
	mp->pos_info_out.feat_infos[mp->pos_info_out.num].word_infos[0].num = 1;

	//LEMMA
	skip_and_read(buffer, fname, fp);
	mp->lemma_num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "lemma_num", fname));
	check_if_exceeds(mp->lemma_num, "lemma_num", MAX_LEMMA_SIZE, "MAX_LEMMA_SIZE");

	//DEPREL
	skip_and_read(buffer, fname, fp);
	mp->deprel_num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "deprel_num", fname));
	check_if_exceeds(mp->deprel_num, "deprel_num", MAX_DEPREL_SIZE, "MAX_DEPREL_SIZE");

	//ROOT DEPREL ID
	skip_and_read(buffer, fname, fp);
	int root_id = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "ROOT_DEPREL", fname));
	if (root_id != ROOT_DEPREL) {
		fprintf(stderr, "Error: wrong id for  ROOT_DEPREL in '%s',  %d instead of %d\n", fname, root_id, ROOT_DEPREL);
		exit(1);
	}


	strncpy(mp->s_deprel_root, field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "s_deprel_root", fname), MAX_REC_LEN);

	int d;
	for (d = 0; d < mp->deprel_num; d++) {
		skip_and_read(buffer, fname, fp);
		int id = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "deprel", fname));
		if (id != d) {
			fprintf(stderr, "Error: wrong id for deprel in '%s',  %d instead of %d\n", fname, id, d);
			exit(1);
		}
		char *s_deprel = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "s_deprel", fname);
		strncpy(mp->s_deprel[d], s_deprel, MAX_REC_LEN);
	}

	//ELFEAT
	skip_and_read(buffer, fname, fp);
	mp->elfeat_num = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "elfeat_num", fname));
	check_if_exceeds(mp->elfeat_num, "elfeat_num", MAX_ELFEAT_SIZE, "MAX_ELFEAT_SIZE");



	ALLOC_ARRAY(mp->sense_num, int *, BANK_NUM);
	ALLOC_ARRAY(mp->s_sense, char***, BANK_NUM);

	int bank;
	for (bank = 0; bank < BANK_NUM; bank++) {

		ALLOC_ARRAY(mp->sense_num[bank], int, mp->lemma_num);
		ALLOC_ARRAY(mp->s_sense[bank], char**, mp->lemma_num);

		skip_and_read(buffer, fname, fp);
		int predic_lemma_num =  atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "predic_lemma_num", fname));
		//TMP check_if_exceeds(predic_lemma_num, "predic_lemma_num", mp->lemma_num, "lemma_num");

		int l;
		for (l = 0; l < predic_lemma_num; l++) {
			skip_and_read(buffer, fname, fp);
			int lemma = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "lemma", fname));
			//s_lemma - do not care
			field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "s_lemma", fname);
			mp->sense_num[bank][lemma] = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "sense_num", fname));
			ALLOC_ARRAY(mp->s_sense[bank][lemma], char*, mp->sense_num[bank][lemma]);
			int s;
			for (s = 0; s < mp->sense_num[bank][lemma]; s++) {
				skip_and_read(buffer, fname, fp);
				int sense = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "sense", fname));
				if (sense != s) { fprintf(stderr, "Error: senses are not in numerical order"); exit(1);}
				char *s_sense = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "s_sense", fname);
				ALLOC_ARRAY(mp->s_sense[bank][lemma][sense], char, strlen(s_sense) + 1);
				strcpy(mp->s_sense[bank][lemma][sense], s_sense);
			}

		} // different lemmas

		skip_and_read(buffer, fname, fp);
		mp->role_num[bank] = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "role_num", fname));

		int r;
		for (r = 0; r < mp->role_num[bank]; r++) {
			skip_and_read(buffer, fname, fp);
			int role = atoi(field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "role", fname));
			if (role != r) { fprintf(stderr, "Error: roles are not in numerical order"); exit(1);}
			char *s_role = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "s_role", fname);
			strncpy(mp->s_arg_role[bank][r], s_role, MAX_REC_LEN);
		}
	}


	fclose(fp);

	return;
}

int is_empty(char *s) {
	ASSERT(s != NULL);
	int i;
	for (i = 0; i < strlen(s); i++) {
		if (s[i] != ' ' && s[i] != '\t' && s[i] != '\n' && s[i] != '\r') {
			return 0;
		}
	}

	return 1;
}
void _read_node_route(MODEL_PARAMS *mp, NODE_ROUTE *node, char *fname) {
	char *src = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "src", fname);
	if (strcmp("STACK", src) == 0) {
		node->src = STACK_SRC;
	} else if (strcmp("SRL_STACK", src) == 0) {
		node->src = SRL_STACK_SRC;
	} else if (strcmp("INPUT", src) == 0) {
		node->src = INP_SRC;
	} else {
		fprintf(stderr, "Error: value '%s' is wrong for the third field in file '%s'\n", src, fname);
		exit(1);
	}

	node->offset_curr = opt_int_field_wrapper(strtok(NULL, FIELD_SEPS));
	node->offset_orig = opt_int_field_wrapper(strtok(NULL, FIELD_SEPS));

	char *role = strtok(NULL, FIELD_SEPS);
	if (role == NULL || strcmp(role, "_") == 0 ) {
		node->arg_bank = -1;
		node->arg_role = -1;

	} else {
		char *save_ptr = NULL;
		node->arg_bank = atoi(field_missing_wrapper_ext(strtok_r(role, ":", &save_ptr), "arg_bank", fname));
		if (node->arg_bank < 0 || node->arg_bank >= BANK_NUM) {
			fprintf(stderr, "Error: value of arg_bank (%d) in file '%s' is wrong, should be between 0 and %d; or set field to '_'\n", node->arg_bank, fname,  BANK_NUM - 1);
			exit(1);
		}
		char * s_role = field_missing_wrapper_ext(strtok_r(NULL, ":", &save_ptr), "arg_role", fname);
		node->arg_role = -1;
		int i;
		for (i = 0; i < mp->role_num[node->arg_bank]; i++) {
			if (strcmp(s_role, mp->s_arg_role[node->arg_bank][i]) == 0) {
				node->arg_role = i;
			}
		}
		if (node->arg_role < 0) {
			fprintf(stderr, "Error: unknown role '%s' in file '%s'\n", s_role, fname);
			exit(1);
		}
	}

	node->head = opt_int_field_wrapper(strtok(NULL, FIELD_SEPS));
	node->rc = opt_int_field_wrapper(strtok(NULL, FIELD_SEPS));
	node->rs = opt_int_field_wrapper(strtok(NULL, FIELD_SEPS));
}


//reads and reserves space in mp for corresponding weights
int  read_ih_links_spec(char *fname, MODEL_PARAMS *mp, IH_LINK_SPEC *specs, int *num) {
	FILE *fp;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}

	char buffer[MAX_LINE];

	int links_before = *num ;//mp->ih_links_num;
	while (fgets(buffer, MAX_LINE, fp) != NULL) {
		while (buffer[0] == '#') {
			if (fgets(buffer, MAX_LINE, fp) == NULL) {
				break;
			}
		}
		if (is_empty(buffer)) {
			continue;
		}

		IH_LINK_SPEC *ih_spec = &(specs[(*num)++]);   //&(mp->ih_links_specs[mp->ih_links_num++])

		ih_spec->node.src = -1;
		ih_spec->node_other.src = -1;

		char *step_type = field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "step_type", fname);
		if (strcmp("SYNT_STEP", step_type) == 0) {
			ih_spec->step_type = SYNT_STEP;
		} else if (strcmp("SRL_STEP", step_type) == 0) {
			ih_spec->step_type = SRL_STEP;
		} else if (strcmp("ANY_STEP", step_type) == 0) {
			ih_spec->step_type = ANY_STEP;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for the first field in file '%s'\n", step_type, fname);
			exit(1);
		}

		char *feature_class = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "feature_class", fname);
		if (strcmp("NODE", feature_class) == 0) {
			ih_spec->feature_class = NODE_FEATURE_CLASS;
		} else if (strcmp("PATH", feature_class) == 0) {
			ih_spec->feature_class = PATH_FEATURE_CLASS;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong  - no such feature class supported, file '%s'\n", step_type, fname);
		}


		char *type = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "info_type", fname);
		if (ih_spec->feature_class == NODE_FEATURE_CLASS) {
			if (strcmp("LEX", type) == 0) {
				ih_spec->info_type = WORD_TYPE;
			} else if (strcmp("POS", type) == 0) {
				ih_spec->info_type = POS_TYPE;
			} else if (strcmp("DEP", type) == 0) {
				ih_spec->info_type = DEPREL_TYPE;
			} else if (strcmp("LEMMA", type) == 0) {
				ih_spec->info_type = LEMMA_TYPE;
			} else if (strcmp("CPOS", type) == 0) {
				ih_spec->info_type = CPOS_TYPE;
			} else if (strcmp("FEATS", type) == 0) {
				ih_spec->info_type = FEAT_TYPE;
			} else if (strcmp("SENSE", type) == 0) {
				ih_spec->info_type = SENSE_TYPE;
			} else {
				fprintf(stderr, "Error: value '%s' is wrong for the second field in file '%s'\n", type, fname);
				exit(1);
			}
		} else if (ih_spec->feature_class == PATH_FEATURE_CLASS) {
			if (strcmp("DIR_SYNT_LAB", type) == 0) {
				ih_spec->info_type = SYNT_LAB_PATH_TYPE;
			} else {
				fprintf(stderr, "Error: value '%s' is wrong for the second field in file '%s'\n", type, fname);
				exit(1);
			}
		}

		_read_node_route(mp, &ih_spec->node, fname);
		if (ih_spec->feature_class == PATH_FEATURE_CLASS) {
			_read_node_route(mp, &ih_spec->node_other, fname);
		}

		if (*num >= MAX_IH_LINKS) {
			fprintf(stderr, "Number of input links exceeded MAX_IH_LINKS = %d\n", MAX_IH_LINKS);
			exit(1);
		}
	}

	fclose(fp);
	return *num - links_before;
}


//reads and reserves space in mp for corresponding weights
int read_hh_links_spec(char *fname, MODEL_PARAMS *mp) {
	FILE *fp;
	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", fname);
		exit(1);
	}

	char buffer[MAX_LINE];

	int links_before = mp->hh_links_num;

	while (fgets(buffer, MAX_LINE, fp) != NULL) {
		while (buffer[0] == '#') {
			if (fgets(buffer, MAX_LINE, fp) == NULL) {
				break;
			}
		}
		if (is_empty(buffer)) {
			continue;
		}

		HH_LINK_SPEC *hh_spec = &(mp->hh_links_specs[mp->hh_links_num++]);


		char *when = field_missing_wrapper_ext(strtok(buffer, FIELD_SEPS), "when", fname);
		if (strcmp("FIND_FIRST", when) == 0) {
			hh_spec->when = FIND_FIRST;
		} else if (strcmp("FIND_LAST", when) == 0) {
			hh_spec->when = FIND_LAST;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for the first field in file '%s'\n", when, fname);
			exit(1);
		}

		char *orig_step_type = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_step_type", fname);
		if (strcmp("SYNT_STEP", orig_step_type) == 0) {
			hh_spec->orig_step_type = SYNT_STEP;
		} else if (strcmp("SRL_STEP", orig_step_type) == 0) {
			hh_spec->orig_step_type = SRL_STEP;
		} else if (strcmp("ANY_STEP", orig_step_type) == 0) {
			hh_spec->orig_step_type = ANY_STEP;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for the second field in file '%s'\n", orig_step_type, fname);
			exit(1);
		}

		char *src = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_src", fname);
		if (strcmp("STACK", src) == 0) {
			hh_spec->orig_src = STACK_SRC;
		} else if (strcmp("SRL_STACK", src) == 0) {
			hh_spec->orig_src = SRL_STACK_SRC;
		} else if (strcmp("INPUT", src) == 0) {
			hh_spec->orig_src = INP_SRC;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for the third field in file '%s'\n", src, fname);
			exit(1);
		}

		hh_spec->orig_offset = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_offset", fname));

		char *role = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "arg_role", fname);
		if (strcmp(role, "_") == 0 ) {
			hh_spec->orig_arg_bank = -1;
			hh_spec->orig_arg_role = -1;
		} else {
			char *save_ptr;
			hh_spec->orig_arg_bank = atoi(field_missing_wrapper_ext(strtok_r(role, ":", &save_ptr), "arg_bank", fname));
			if (hh_spec->orig_arg_bank < 0 || hh_spec->orig_arg_bank >= BANK_NUM) {
				fprintf(stderr, "Error: value of arg_bank (%d) in file '%s' is wrong, should be between 0 and %d; or set field to '_'\n", hh_spec->orig_arg_bank, fname,  BANK_NUM - 1);
				exit(1);
			}
			char * s_role = field_missing_wrapper_ext(strtok_r(NULL, ":", &save_ptr), "arg_role", fname);
			hh_spec->orig_arg_role = -1;
			int i;
			for (i = 0; i < mp->role_num[hh_spec->orig_arg_bank]; i++) {
				if (strcmp(s_role, mp->s_arg_role[hh_spec->orig_arg_bank][i]) == 0) {
					hh_spec->orig_arg_role = i;
				}
			}
			if (hh_spec->orig_arg_role < 0) {
				fprintf(stderr, "Error: unknown role '%s' in file '%s'\n", s_role, fname);
				exit(1);
			}
		}

		hh_spec->orig_head = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_head", fname));
		hh_spec->orig_rrc = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_rrc", fname));
		hh_spec->orig_rs = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "orig_rs", fname));


		char *target_step_type = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "target_step_type", fname);
		if (strcmp("SYNT_STEP", target_step_type) == 0) {
			hh_spec->target_step_type = SYNT_STEP;
		} else if (strcmp("SRL_STEP", target_step_type) == 0) {
			hh_spec->target_step_type = SRL_STEP;
		} else if (strcmp("ANY_STEP", target_step_type) == 0) {
			hh_spec->target_step_type = ANY_STEP;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for the second field in file '%s'\n", target_step_type, fname);
			exit(1);
		}

		src = field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "target_src", fname);
		if (strcmp("STACK", src) == 0) {
			hh_spec->target_src = STACK_SRC;
		} else if (strcmp("SRL_STACK", src) == 0) {
			hh_spec->target_src = SRL_STACK_SRC;
		} else if (strcmp("INPUT", src) == 0) {
			hh_spec->target_src = INP_SRC;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong in file '%s'\n", src, fname);
			exit(1);
		}

		hh_spec->target_offset = atoi(field_missing_wrapper_ext(strtok(NULL, FIELD_SEPS), "target_offset", fname));

		char *act = strtok(NULL, FIELD_SEPS);

		if (act == NULL) {
			act = "ANY";
		}

		if (strcmp("LA", act) == 0) {
			hh_spec->act = LA;
		} else if (strcmp("RA", act) == 0) {
			hh_spec->act = RA;
		} else if (strcmp("SHIFT", act) == 0) {
			hh_spec->act = SHIFT;
		} else if (strcmp("RED", act) == 0) {
			hh_spec->act = RED;
		} else if (strcmp("ANY", act) == 0) {
			hh_spec->act = ANY;
		} else {
			fprintf(stderr, "Error: value '%s' is wrong for in file '%s'\n", act, fname);
			exit(1);
		}

		char *offset = strtok(NULL, FIELD_SEPS);

		if (offset == NULL) {
			//default offset is any
			hh_spec->offset = -1;
		} else {
			hh_spec->offset = atoi(offset);
		}


		if (mp->hh_links_num >= MAX_HH_LINKS) {
			fprintf(stderr, "Number of input links exceeded MAX_HH_LINKS = %d\n", MAX_HH_LINKS);
			exit(1);
		}
	}
	fclose(fp);
	return mp->hh_links_num - links_before;
}

//allocates memory space for all the weights
//should be invoked after reading hh, ih and io specs
void allocate_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {

	//allocate input links
	int l;

	ALLOC_ARRAY(mp->ih_link_cpos, IH_LINK *, mp->ih_links_num);
	ALLOC_ARRAY(mp->ih_link_deprel, IH_LINK *, mp->ih_links_num);
	ALLOC_ARRAY(mp->ih_link_pos, IH_LINK *, mp->ih_links_num);
	ALLOC_ARRAY(mp->ih_link_lemma, IH_LINK *, mp->ih_links_num);
	if (ap->inp_feat_mode == FEAT_MODE_COMPOSED) {
		ALLOC_ARRAY(mp->ih_link_feat, IH_LINK **, mp->ih_links_num);
	} else if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY) {
		ALLOC_ARRAY(mp->ih_link_elfeat, IH_LINK *, mp->ih_links_num);
	} else if (ap->inp_feat_mode == FEAT_MODE_BOTH) {
		ALLOC_ARRAY(mp->ih_link_feat, IH_LINK **, mp->ih_links_num);
		ALLOC_ARRAY(mp->ih_link_elfeat, IH_LINK *, mp->ih_links_num);
	} else {
		ASSERT(0);
	}
	ALLOC_ARRAY(mp->ih_link_sense, IH_LINK ***, mp->ih_links_num);
	ALLOC_ARRAY(mp->ih_link_word, IH_LINK ***, mp->ih_links_num);


	ALLOC_ARRAY(mp->ih_link_synt_lab_path, IH_LINK *, mp->ih_links_num);


	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			//+1 added: used when  constituent for which dependency is considered  is missing
			//+1 added for ROOT_DEPREL (it is not included in mp->deprel_num)
			ALLOC_ARRAY(mp->ih_link_deprel[l], IH_LINK, mp->deprel_num + 2);
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			//+1 added: used when  constituent for which POS tag is considered  is missing
			ALLOC_ARRAY(mp->ih_link_pos[l], IH_LINK, mp->pos_info_in.num + 1);
		} else if (mp->ih_links_specs[l].info_type == WORD_TYPE) {
			ALLOC_ARRAY(mp->ih_link_word[l], IH_LINK**, mp->pos_info_in.num + 1);
			int pos_idx, feat_idx;
			for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
				ALLOC_ARRAY(mp->ih_link_word[l][pos_idx], IH_LINK*, mp->pos_info_in.feat_infos[pos_idx].num);
				for (feat_idx = 0; feat_idx < mp->pos_info_in.feat_infos[pos_idx].num; feat_idx++) {
					ALLOC_ARRAY(mp->ih_link_word[l][pos_idx][feat_idx], IH_LINK, mp->pos_info_in.feat_infos[pos_idx].word_infos[feat_idx].num);
				}
			}
			//ALLOC item mp->ih_link_word[l][mp->pos_info_in.num][0][0] for no-such word item
			ALLOC_ARRAY(mp->ih_link_word[l][mp->pos_info_in.num], IH_LINK*, 1);
			ALLOC_ARRAY(mp->ih_link_word[l][mp->pos_info_in.num][0], IH_LINK, 1);

		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				ALLOC_ARRAY(mp->ih_link_feat[l], IH_LINK*, mp->pos_info_in.num + 1);
				int pos_idx;
				for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
					ALLOC_ARRAY(mp->ih_link_feat[l][pos_idx], IH_LINK, mp->pos_info_in.feat_infos[pos_idx].num);
				}
				//ALLOC item mp->ih_link_feat[l][mp->pos_info_in.num][0] for no-such word item
				ALLOC_ARRAY(mp->ih_link_feat[l][mp->pos_info_in.num], IH_LINK, 1);
			}
			if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
				ALLOC_ARRAY(mp->ih_link_elfeat[l], IH_LINK, mp->elfeat_num + 1);
			}
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			int b, lemma;
			ALLOC_ARRAY(mp->ih_link_sense[l], IH_LINK**, BANK_NUM + 2);
			for (b = 0; b < BANK_NUM; b++) {
				ALLOC_ARRAY(mp->ih_link_sense[l][b], IH_LINK*, mp->lemma_num);
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					if (mp->sense_num[b][lemma] > 0) {
						ALLOC_ARRAY(mp->ih_link_sense[l][b][lemma], IH_LINK, mp->sense_num[b][lemma]);
					} else {
						mp->ih_link_sense[l][b][lemma] = NULL;
					}
				}
			}

			// allocate no word links and no sense links (no predicate)
			ALLOC_ARRAY(mp->ih_link_sense[l][BANK_NUM], IH_LINK*, 1);
			ALLOC_ARRAY(mp->ih_link_sense[l][BANK_NUM][0], IH_LINK, 1);
			ALLOC_ARRAY(mp->ih_link_sense[l][BANK_NUM + 1], IH_LINK*, 1);
			ALLOC_ARRAY(mp->ih_link_sense[l][BANK_NUM + 1][0], IH_LINK, 1);

		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			ASSERT(mp->ih_links_specs[l].feature_class == PATH_FEATURE_CLASS);
			// 2 - because direction should be captured
			ALLOC_ARRAY(mp->ih_link_synt_lab_path[l], IH_LINK, get_synt_path_feature_size(mp) + 1);
		} else if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			ALLOC_ARRAY(mp->ih_link_cpos[l], IH_LINK, mp->cpos_num + 1);
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			ALLOC_ARRAY(mp->ih_link_lemma[l], IH_LINK, mp->lemma_num + 1);
		} else {
			ASSERT(0);
		}
	}

	//allocate HID-HID weight matrixes
	ALLOC_ARRAY(mp->hh_link, HH_LINK, mp->hh_links_num);


	alloc_out_link_contents(&mp->out_link_act, ACTION_NUM, ap->hid_size);
	alloc_out_link_contents(&mp->out_link_la_label, mp->deprel_num, ap->hid_size);
	alloc_out_link_contents(&mp->out_link_ra_label, mp->deprel_num, ap->hid_size);


	//allocate output weights
	ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
	alloc_out_link_contents(&mp->out_link_pos, 1 + mp->pos_info_out.num, ap->hid_size);

	ALLOC_ARRAY(mp->out_link_feat, OUT_LINK, mp->pos_info_out.num + 1);
	int p;
	for (p = 0; p <  mp->pos_info_out.num + 1; p++) {
		alloc_out_link_contents(&mp->out_link_feat[p],  mp->pos_info_out.feat_infos[p].num, ap->hid_size);
	}


	ALLOC_ARRAY(mp->out_link_word, OUT_LINK*, mp->pos_info_out.num + 1);

	int f;

	for (p = 0; p < mp->pos_info_out.num; p++) {
		ALLOC_ARRAY(mp->out_link_word[p], OUT_LINK,  mp->pos_info_out.feat_infos[p].num);
		for (f = 0; f <  mp->pos_info_out.feat_infos[p].num; f++) {
			alloc_out_link_contents(&mp->out_link_word[p][f],  mp->pos_info_out.feat_infos[p].word_infos[f].num, ap->hid_size);
		}
	}
	ALLOC_ARRAY(mp->out_link_word[mp->pos_info_out.num], OUT_LINK, 1);
	alloc_out_link_contents(&mp->out_link_word[mp->pos_info_out.num][0], mp->pos_info_out.feat_infos[mp->pos_info_out.num].word_infos[0].num, ap->hid_size);

	ALLOC_ARRAY(mp->out_link_sem_la_label, OUT_LINK, BANK_NUM);
	ALLOC_ARRAY(mp->out_link_sem_ra_label, OUT_LINK, BANK_NUM);
	int b;
	for (b = 0; b < BANK_NUM; b++) {
		alloc_out_link_contents(&mp->out_link_sem_la_label[b], mp->role_num[b], ap->hid_size);
		alloc_out_link_contents(&mp->out_link_sem_ra_label[b], mp->role_num[b], ap->hid_size);
	}

	ALLOC_ARRAY(mp->out_link_sense, OUT_LINK**, BANK_NUM);


	for (b = 0; b < BANK_NUM; b++) {
		ALLOC_ARRAY(mp->out_link_sense[b], OUT_LINK*, mp->lemma_num);
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			if (mp->sense_num[b][l] == 0) continue;
			ALLOC_ARRAY(mp->out_link_sense[b][l], OUT_LINK, 1);
			alloc_out_link_contents(&mp->out_link_sense[b][l][0], mp->sense_num[b][l], ap->hid_size);
		}
	}
}

void free_weights(MODEL_PARAMS *mp) {
	//TODO: add freeing contents of OUT_LINK

	//free input links
	int l;
	for (l = 0; l < mp->ih_links_num; l++) {
		if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
			free(mp->ih_link_deprel[l]);
		} else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
			free(mp->ih_link_pos[l]);
		} else if (mp->ih_links_specs[l].info_type == WORD_TYPE) {
			int pos_idx, feat_idx;
			for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
				for (feat_idx = 0; feat_idx < mp->pos_info_in.feat_infos[pos_idx].num; feat_idx++) {
					free(mp->ih_link_word[l][pos_idx][feat_idx]);
				}
				free(mp->ih_link_word[l][pos_idx]);
			}
			free(mp->ih_link_word[l][mp->pos_info_in.num][0]);
			free(mp->ih_link_word[l][mp->pos_info_in.num]);
			free(mp->ih_link_word[l]);
		} else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
			int b, lemma;
			for (b = 0; b < BANK_NUM; b++) {
				for (lemma = 0; lemma < mp->lemma_num; lemma++) {
					if (mp->sense_num[b][lemma] > 0) {
						free(mp->ih_link_sense[l][b][lemma]);
					}
				}
				free(mp->ih_link_sense[l][b]);
			}

			// allocate no word links
			free(mp->ih_link_sense[l][BANK_NUM][0]);
			free(mp->ih_link_sense[l][BANK_NUM]);
			free(mp->ih_link_sense[l][BANK_NUM + 1][0]);
			free(mp->ih_link_sense[l][BANK_NUM + 1]);
			free(mp->ih_link_sense[l]);

		} else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
			if (mp->ih_link_feat != NULL) {
				int pos_idx;
				for (pos_idx = 0; pos_idx < mp->pos_info_in.num; pos_idx++) {
					free(mp->ih_link_feat[l][pos_idx]);
				}
				free(mp->ih_link_feat[l][mp->pos_info_in.num]);
				free(mp->ih_link_feat[l]);
			}
			if (mp->ih_link_elfeat != NULL) {
				free(mp->ih_link_elfeat[l]);
			}

		} else if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
			free(mp->ih_link_cpos[l]);
		} else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
			free(mp->ih_link_lemma[l]);
		} else if (mp->ih_links_specs[l].info_type == SYNT_LAB_PATH_TYPE) {
			free(mp->ih_link_synt_lab_path[l]);
		} else {
			ASSERT(0);
		}
	}

	free(mp->ih_link_cpos);
	free(mp->ih_link_deprel);
	free(mp->ih_link_pos);
	free(mp->ih_link_lemma);
	if (mp->ih_link_feat != NULL) {
		free(mp->ih_link_feat);
	}
	if (mp->ih_link_elfeat != NULL) {
		free(mp->ih_link_elfeat);
	}
	free(mp->ih_link_word);
	free(mp->ih_link_sense);
	free(mp->ih_link_synt_lab_path);

	//free HID-HID weight matrixes
	free(mp->hh_link);

	//free output weights
	free(mp->out_link_feat);

	int pos_idx;
	for (pos_idx = 0; pos_idx < mp->pos_info_out.num; pos_idx++) {
		free(mp->out_link_word[pos_idx]);
	}
	free(mp->out_link_word[mp->pos_info_out.num]);
	free(mp->out_link_word);

	int b;
	for (b = 0; b < BANK_NUM; b++) {
		int l;
		for (l = 0; l < mp->lemma_num; l++) {
			int s;
			for (s = 0; s < mp->sense_num[b][l]; s++) {
				free(mp->s_sense[b][l][s]);
			}
			if (mp->sense_num[b][l] > 0) {
				free(mp->s_sense[b][l]);
			}
		}
		free(mp->sense_num[b]);
		free(mp->s_sense[b]);
	}
	free(mp->sense_num);
	free(mp->s_sense);

}

int get_matched_syntax(SENTENCE *sent1, SENTENCE *sent2, int labeled) {
	if (sent1->len != sent2->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent1->len, sent2->len);
		exit(-1);
	}
	int t, matched = 0;
	for (t = 1; t < sent1->len + 1; t++) {
		ASSERT(strcmp(sent1->s_word[t], sent2->s_word[t]) == 0);
		if (labeled) {
			if ((sent1->head[t] == sent2->head[t]) && (strcmp(sent1->s_deprel[t], sent2->s_deprel[t]) == 0))
				matched++;
		} else {
			if (sent1->head[t] == sent2->head[t])
				matched++;
		}
	}
	return matched;
}


int get_match_roles(SENTENCE *sent1, SENTENCE *sent2, int w) {
	int i, j, matched = 0;

	for (i = 0; i < sent1->arg_num[w]; i++) {
		for (j = 0; j < sent2->arg_num[w]; j++) {
			if (sent1->args[w][i] == sent2->args[w][j] && strcmp(sent1->s_arg_roles[w][i], sent2->s_arg_roles[w][j]) == 0)
				matched++;
		}
	}

	ASSERT(matched <= sent1->arg_num[w] && matched <= sent2->arg_num[w]);
	return matched;
}

/* official SRL scoring function */
void add_matched_srl(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval) {
	if (sent_mod->len != sent_gld->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent_mod->len, sent_gld->len);
		exit(-1);
	}
	int t;
	for (t = 1; t < sent_mod->len + 1; t++) {
		ASSERT(strcmp(sent_mod->s_word[t], sent_gld->s_word[t]) == 0);

		// compute sense scores
		if (sent_gld->sense[t] >= 0) {
			eval->gold++;
		}
		if (sent_mod->sense[t] >= 0) {
			eval->model++;
			if (sent_gld->sense[t] >= 0) {
				eval->matched += (strcmp(sent_mod->s_sense[t], sent_gld->s_sense[t]) == 0);
			}
		}

		// compute argument scores
		eval->gold += sent_gld->arg_num[t];
		eval->model += sent_mod->arg_num[t];
		eval->matched += get_match_roles(sent_gld, sent_mod, t);
	}
}

void add_matched_roles(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval) {
	if (sent_mod->len != sent_gld->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent_mod->len, sent_gld->len);
		exit(-1);
	}
	int t;
	for (t = 1; t < sent_mod->len + 1; t++) {
		ASSERT(strcmp(sent_mod->s_word[t], sent_gld->s_word[t]) == 0);

		// compute argument scores
		eval->gold += sent_gld->arg_num[t];
		eval->model += sent_mod->arg_num[t];
		eval->matched += get_match_roles(sent_gld, sent_mod, t);
	}
}

void add_matched_predicates(SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval) {
	if (sent_mod->len != sent_gld->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent_mod->len, sent_gld->len);
		exit(-1);
	}
	int t;
	for (t = 1; t < sent_mod->len + 1; t++) {
		ASSERT(strcmp(sent_mod->s_word[t], sent_gld->s_word[t]) == 0);

		// compute sense scores
		if (sent_gld->sense[t] >= 0) {
			eval->gold++;
		}
		if (sent_mod->sense[t] >= 0) {
			eval->model++;
			if (sent_gld->sense[t] >= 0) {
				if (strcmp(sent_mod->s_sense[t], sent_gld->s_sense[t]) == 0)
					eval->matched++;
			}
		}
	}
}

void add_matched_bank_roles(int bank, SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval) {
	if (sent_mod->len != sent_gld->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent_mod->len, sent_gld->len);
		exit(-1);
	}
	int t;
	for (t = 1; t < sent_mod->len + 1; t++) {
		ASSERT(strcmp(sent_mod->s_word[t], sent_gld->s_word[t]) == 0);

		// compute argument scores
		if (sent_gld->bank[t] == bank) {
			eval->gold += sent_gld->arg_num[t];
		}
		if (sent_mod->bank[t] == bank) {
			eval->model += sent_mod->arg_num[t];
			if (sent_gld->bank[t] == bank) {
				eval->matched += get_match_roles(sent_gld, sent_mod, t);
			}
		}
	}
}

void add_matched_bank_predicates(int bank, SENTENCE *sent_mod, SENTENCE *sent_gld, EVAL *eval) {
	if (sent_mod->len != sent_gld->len) {
		fprintf(stderr, "Error: comparing sentences of different length: %d vs %d\n", sent_mod->len, sent_gld->len);
		exit(-1);
	}
	int t;
	for (t = 1; t < sent_mod->len + 1; t++) {
		ASSERT(strcmp(sent_mod->s_word[t], sent_gld->s_word[t]) == 0);

		// compute sense scores
		if (sent_gld->sense[t] >= 0 && sent_gld->bank[t] == bank) {
			eval->gold++;
		}
		if (sent_mod->sense[t] >= 0 && sent_mod->bank[t] == bank) {
			eval->model++;
			if (sent_gld->sense[t] >= 0 && sent_gld->bank[t] == bank) {
				if (strcmp(sent_mod->s_sense[t], sent_gld->s_sense[t]) == 0)
					eval->matched++;
			}
		}
	}
}


void print_state(char *s, APPROX_PARAMS *ap) {
	cat_print_s(s, "WHERE", ap->st.where);
	cat_print_f(s, "CURR_REG", ap->st.curr_reg);
	cat_print_f(s, "CURR_ETA", ap->st.curr_eta);
	cat_print_f(s, "MAX_SCORE", ap->st.max_score);
	cat_print_f(s, "LAST_SCORE", ap->st.last_score);
	cat_print_f(s, "LAST_TR_ERR", ap->st.last_tr_err);
	cat_print_i(s, "LOOPS_NUM", ap->st.loops_num);
	cat_print_i(s, "NUM_SUB_OPT", ap->st.num_sub_opt);
}


void save_state(APPROX_PARAMS *ap) {
	char state_fname[MAX_NAME];
	strcpy(state_fname, ap->model_name);
	strcat(state_fname, ".state");

	FILE *fp;
	if ((fp = fopen(state_fname, "w")) == NULL) {
		fprintf(stderr, "Error: can't open file %s\n", state_fname);
		exit(1);
	}
	DEF_ALLOC_ARRAY(s, char,  sizeof(char) * MAX_LINE * 25);

	print_state(s, ap);

	fprintf(fp, s);
	free(s);
	fclose(fp);
}

int load_state(APPROX_PARAMS *ap) {
	char state_fname[MAX_NAME];
	strcpy(state_fname, ap->model_name);
	strcat(state_fname, ".state");

	FILE *fp;
	if ((fp = fopen(state_fname, "r")) == NULL) {
		return 0;
	}

	char buffer[MAX_LINE], *s;
	int line = 0;
	do {
		do {
			s = fgets(buffer, MAX_LINE, fp);
			if (s == NULL) {
				break;
			}
			line++;
		} while (buffer[0] == '#' || buffer[0] == '\0');
		if (s == NULL) {
			break;
		}

		char *field = strtok(buffer, FIELD_SEPS);
		if (field == NULL) {
			continue;
		}

		IF_EQ_SET_N_CONT_S("WHERE", ap->st.where);
		IF_EQ_SET_N_CONT_F("CURR_REG", ap->st.curr_reg);
		IF_EQ_SET_N_CONT_F("CURR_ETA", ap->st.curr_eta);
		IF_EQ_SET_N_CONT_F("MAX_SCORE", ap->st.max_score);
		IF_EQ_SET_N_CONT_F("LAST_SCORE", ap->st.last_score);
		IF_EQ_SET_N_CONT_F("LAST_TR_ERR", ap->st.last_tr_err);
		IF_EQ_SET_N_CONT_I("LOOPS_NUM", ap->st.loops_num);
		IF_EQ_SET_N_CONT_I("NUM_SUB_OPT", ap->st.num_sub_opt);


		fprintf(stderr, "Error: unknown field '%s' in the state file\n", field);
		exit(1);
	} while (1);
	fclose(fp);
	return 1;

}


void save_candidates(APPROX_PARAMS *ap, MODEL_PARAMS *mp, SENTENCE *inp_sent, OPTION **best_options, int avail_num) {
	if (!ap->return_cand) {
		return;
	}

	char cand_fname[MAX_NAME];
	strcpy(cand_fname, ap->out_file);
	strcat(cand_fname, ".top");

	DEF_FOPEN(fp, cand_fname, "a");

	int num = ap->cand_num <= avail_num ? ap->cand_num : avail_num;

	fprintf(fp, "%d\n\n", num);
	int i;
	for (i = 0; i < num; i++) {
		DEF_ALLOC(sent, SENTENCE);
		memcpy(sent, inp_sent, sizeof(SENTENCE));
		pt_fill_sentence(best_options[i]->pt, mp, sent);
		char *s = print_sent(sent, 1);
		fprintf(fp, "%e\n%s\n", best_options[i]->lprob, s);
		free(s);
		free(sent);
	}

	fclose(fp);
}

#define REWR_IF_EQ_SET_N_CONT_S(name, var) \
	{\
	char s[MAX_NAME]; \
	strcpy(s, argv[i]);\
	char *val = index(s, '='); \
	if (val == NULL) {\
		fprintf(stderr, "Error: Wrong parameter '%s'\n", argv[i]);\
		exit(1);\
	}\
	val[0] = '\0'; \
	val++;\
	if (strcmp(s, name) == 0) {\
		strcpy(var, val);\
		continue;\
	}\
	}
#define REWR_IF_EQ_SET_N_CONT_I(name, var) \
	{\
	char s[MAX_NAME]; \
	strcpy(s, argv[i]);\
	char *val = index(s, '='); \
	if (val == NULL) {\
		fprintf(stderr, "Error: Wrong parameter '%s'\n", argv[i]);\
		exit(1);\
	}\
	val[0] = '\0'; \
	val++;\
	if (strcmp(s, name) == 0) {\
		var = atoi(val);\
		continue;\
	}\
	}
#define REWR_IF_EQ_SET_N_CONT_F(name, var) \
	{\
	char s[MAX_NAME]; \
	strcpy(s, argv[i]);\
	char *val = index(s, '='); \
	if (val == NULL) {\
		fprintf(stderr, "Error: Wrong parameter '%s'\n", argv[i]);\
		exit(1);\
	}\
	val[0] = '\0'; \
	val++;\
	if (strcmp(s, name) == 0) {\
		var = atof(val);\
		continue;\
	}\
	}

void rewrite_parameters(APPROX_PARAMS *ap, int argc, char** argv, int start_idx) {

	int i;
	for (i = start_idx; i < argc; i++) {

		char field[MAX_NAME];
		strcpy(field, argv[i]);
		char *type = index(field, '=');
		if (type == NULL) {
			fprintf(stderr, "Error: Wrong parameter '%s'\n", argv[i]);
			exit(1);
		}
		type[0] = '\0';
		type++;
		if (strcmp(field, "APPROX_TYPE") == 0) {
			if (strcmp(type, "FF") == 0) {
				ap->approx_type = FF;
			} else if (strcmp(type, "MF") == 0) {
				ap->approx_type = MF;
			} else {
				fprintf(stderr, "Wrong value '%s' for 'APPROX_TYPE' field\n", type);
				exit(1);
			}
			continue;
		}


		REWR_IF_EQ_SET_N_CONT_S("MODEL_NAME", ap->model_name);
		REWR_IF_EQ_SET_N_CONT_S("TRAIN_FILE", ap->train_file);
		REWR_IF_EQ_SET_N_CONT_I("TRAIN_DATASET_SIZE", ap->train_dataset_size);
		REWR_IF_EQ_SET_N_CONT_S("TEST_FILE", ap->test_file);
		REWR_IF_EQ_SET_N_CONT_I("TEST_DATASET_SIZE", ap->test_dataset_size);
		REWR_IF_EQ_SET_N_CONT_S("OUT_FILE", ap->out_file);
		REWR_IF_EQ_SET_N_CONT_S("IO_SPEC_FILE", ap->io_spec_file);
		REWR_IF_EQ_SET_N_CONT_I("INP_FEAT_MODE", ap->inp_feat_mode);
		REWR_IF_EQ_SET_N_CONT_S("IH_LINK_SPEC_FILE",ap->ih_links_spec_file);
		REWR_IF_EQ_SET_N_CONT_S("HH_LINK_SPEC_FILE",ap->hh_links_spec_file);
		REWR_IF_EQ_SET_N_CONT_I("INPUT_OFFSET", ap->input_offset);
		REWR_IF_EQ_SET_N_CONT_I("PARSING_MODE", ap->parsing_mode);
		REWR_IF_EQ_SET_N_CONT_I("SRL_EARLY_REDUCE", ap->is_srl_early_reduce);
		REWR_IF_EQ_SET_N_CONT_I("SYNT_EARLY_REDUCE", ap->is_synt_early_reduce);
		REWR_IF_EQ_SET_N_CONT_I("SRL_DEPROJECTIVIZATION", ap->intern_srl_deproj);
		REWR_IF_EQ_SET_N_CONT_I("SYNT_DEPROJECTIVIZATION", ap->intern_synt_deproj);

		REWR_IF_EQ_SET_N_CONT_I("BEAM", ap->beam);
		REWR_IF_EQ_SET_N_CONT_I("SEARCH_BR_FACTOR", ap->search_br_factor);
		REWR_IF_EQ_SET_N_CONT_I("RETURN_CAND", ap->return_cand);
		REWR_IF_EQ_SET_N_CONT_I("CAND_NUM", ap->cand_num);

		REWR_IF_EQ_SET_N_CONT_I("SEED", ap->seed);
		REWR_IF_EQ_SET_N_CONT_F("RAND_RANGE", ap->rand_range);
		REWR_IF_EQ_SET_N_CONT_F("EM_RAND_RANGE", ap->em_rand_range);

		REWR_IF_EQ_SET_N_CONT_I("HID_SIZE", ap->hid_size);

		REWR_IF_EQ_SET_N_CONT_F("INIT_ETA", ap->init_eta);
		REWR_IF_EQ_SET_N_CONT_F("ETA_RED_RATE", ap->eta_red_rate);
		REWR_IF_EQ_SET_N_CONT_F("MAX_ETA_RED", ap->max_eta_red);

		REWR_IF_EQ_SET_N_CONT_F("INIT_REG", ap->init_reg);
		REWR_IF_EQ_SET_N_CONT_F("REG_RED_RATE", ap->reg_red_rate);
		REWR_IF_EQ_SET_N_CONT_F("MAX_REG_RED", ap->max_reg_red);

		REWR_IF_EQ_SET_N_CONT_F("MOM", ap->mom);

		REWR_IF_EQ_SET_N_CONT_I("MAX_LOOPS", ap->max_loops);
		REWR_IF_EQ_SET_N_CONT_I("MAX_LOOPS_WO_ACCUR_IMPR", ap->max_loops_wo_accur_impr);
		REWR_IF_EQ_SET_N_CONT_I("LOOPS_BETWEEN_VAL", ap->loops_between_val);
		REWR_IF_EQ_SET_N_CONT_I("DISTINGUISH_SRL_AND_SYNT_BIAS", ap->disting_biases);

		REWR_IF_EQ_SET_N_CONT_F("MBR_COEFF", ap->mbr_coeff);

		REWR_IF_EQ_SET_N_CONT_I("DD_KERNEL_TYPE", ap->dd_kernel_type);
		REWR_IF_EQ_SET_N_CONT_I("USE_RERANK_DD_COMP", ap->use_dd_comp);
		REWR_IF_EQ_SET_N_CONT_I("USE_RERANK_EXPL_COMP", ap->use_expl_comp);
		REWR_IF_EQ_SET_N_CONT_F("RERANK_LPROB_W", ap->lprob_w);
		REWR_IF_EQ_SET_N_CONT_F("RERANK_DD_W", ap->dd_w);

		fprintf(stderr, "Error: unknown field '%s' in the command line\n", argv[i]);
		exit(1);
	}

	if (ap->hid_size > MAX_HID_SIZE) {
		fprintf(stderr, "Error: HID_SIZE (%d) exceeds MAX_HID_SIZE (%d), try adjusting MAX_HID_SIZE and rebuilding the parser\n",
				ap->hid_size, MAX_HID_SIZE);
		exit(1);
	}
	return;
}




