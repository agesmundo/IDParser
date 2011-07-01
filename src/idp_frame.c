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

#define PROGRESSDOT 0 /* print a "." after processinging each sentence. */


void save_stata(APPROX_PARAMS *ap, char *s) {
    char prog_fname[MAX_NAME];
    strcpy(prog_fname, ap->model_name);
    strcat(prog_fname, ".prog");

    DEF_FOPEN(fp, prog_fname, "w");

    fprintf(fp, s);
    fclose(fp);

}

void remove_stata(APPROX_PARAMS *ap) {
    char prog_fname[MAX_NAME];
    strcpy(prog_fname, ap->model_name);
    strcat(prog_fname, ".prog");
    remove(prog_fname);
}



double get_overall_score(EVAL eval, double synt_las) {
    double ov_p = 0.5 * synt_las  + 0.5 * eval.matched / ((double) eval.model);
    double ov_r = 0.5 * synt_las  + 0.5 * eval.matched / ((double) eval.gold);
    return 2 * ov_p * ov_r /  (ov_p + ov_r);
}

double testing_epoch(APPROX_PARAMS *p, MODEL_PARAMS *w) {
    int i;
    char buffer[MAX_LINE];
    DEF_FOPEN(ofp, p->out_file, "w");

    SENTENCE *gld_sent;
    int matched = 0, all = 0;
    double err = 0.;
    int c = 0;
    printf("Parsing...\n");
    int is_blind = is_blind_file(p->test_file);

    EVAL sem_eval;
    sem_eval.model = 0; sem_eval.gold = 0; sem_eval.matched = 0;
    EVAL propbank_role_eval, nombank_role_eval, propbank_pred_eval, nombank_pred_eval;
    propbank_role_eval.model = 0; propbank_role_eval.gold = 0; propbank_role_eval.matched = 0;
    nombank_role_eval.model = 0; nombank_role_eval.gold = 0; nombank_role_eval.matched = 0;
    propbank_pred_eval.model = 0; propbank_pred_eval.gold = 0; propbank_pred_eval.matched = 0;
    nombank_pred_eval.model = 0; nombank_pred_eval.gold = 0; nombank_pred_eval.matched = 0;

    DEF_FOPEN(ifp, p->test_file, "r");
    while ((gld_sent = read_sentence(w, ifp, !is_blind)) != NULL) {
        double synt_las =  matched / ( (double) all);
        double srl_f1 =  2 * sem_eval.matched / ((double) sem_eval.gold + sem_eval.model);
        double overall_score = get_overall_score(sem_eval, synt_las);

        sprintf(buffer, "Parsing sentence %d (%d words processed, current synt-LAS: %f%%, srl-F1: %f%%, overall: %f%% )\n",
                c, all, 100. * synt_las, 100. * srl_f1, 100. * overall_score);
        save_stata(p, buffer);

        DEF_ALLOC(mod_sent, SENTENCE);
        memcpy(mod_sent, gld_sent, sizeof(SENTENCE));
        for (i = 0; i < gld_sent->len + 1; i++) {
            mod_sent->head[i] = -10;
            strcpy(mod_sent->s_deprel[i], "UNKNOWN-DEPREL");
            mod_sent->deprel[i] = -10;

            strcpy(mod_sent->s_sense[i], "UNKNOWN-SENSE");
            mod_sent->sense[i] = -1;
            mod_sent->arg_num[i] = 0;
        }
        err += label_sentence(p, w, mod_sent);
        save_sentence(ofp, mod_sent, 1);

        all += mod_sent->len;
        matched += get_matched_syntax(mod_sent, gld_sent, 1);
        add_matched_srl(mod_sent, gld_sent, &sem_eval);
        add_matched_bank_predicates(PROP_BANK, mod_sent, gld_sent, &propbank_pred_eval);
        add_matched_bank_predicates(NOM_BANK, mod_sent, gld_sent, &nombank_pred_eval);
        add_matched_bank_roles(PROP_BANK, mod_sent, gld_sent, &propbank_role_eval);
        add_matched_bank_roles(NOM_BANK, mod_sent, gld_sent, &nombank_role_eval);

        free(gld_sent);
        free(mod_sent);
        if  (c != 0 && c % 1 == 0) {
#if PROGRESSDOT
            printf(".");
            fflush(stdout);
#endif
        }
        fflush(stdout);
        c++;
    }
    printf("done. Processed %d sentences\n", c);
    printf("%2d: Testing err: %f\n", p->st.loops_num, err);

    double synt_las = matched / ( (double) all);
    double srl_p =  sem_eval.matched / ((double) sem_eval.model);
    double srl_r =  sem_eval.matched / ((double) sem_eval.gold);
    double srl_f1 =  2 * sem_eval.matched / ((double) sem_eval.gold + sem_eval.model);
    double overall_score;
    if (sem_eval.gold == 0) {
        overall_score = synt_las;
    } else {
        overall_score = get_overall_score(sem_eval, synt_las);
    }

    printf("%2d: Testing accuracy: synt-LAS %f%%  srl-P,R;F1 %.2f%%,%.2f%%;%f%%  overall %f%%\n",
	   p->st.loops_num, 100. * synt_las, 100. * srl_p, 100. * srl_r, 100. * srl_f1, 100. * overall_score);
    printf("%2d:   PB-pred-P,R;F1 %.2f%%,%.2f%%;%.4f%%  NB-pred-P,R;F1 %.2f%%,%.2f%%;%.4f%%\n",
	   p->st.loops_num,
	   100. * propbank_pred_eval.matched / ((double) propbank_pred_eval.model),
	   100. * propbank_pred_eval.matched / ((double) propbank_pred_eval.gold),
	   100. * 2 * propbank_pred_eval.matched / ((double) propbank_pred_eval.gold + propbank_pred_eval.model),
	   100. * nombank_pred_eval.matched / ((double) nombank_pred_eval.model),
	   100. * nombank_pred_eval.matched / ((double) nombank_pred_eval.gold),
	   100. * 2 * nombank_pred_eval.matched / ((double) nombank_pred_eval.gold + nombank_pred_eval.model));
    printf("%2d:   PB-role-P,R;F1 %.2f%%,%.2f%%;%.4f%%  NB-role-P,R;F1 %.2f%%,%.2f%%;%.4f%%\n",
	   p->st.loops_num,
	   100. * propbank_role_eval.matched / ((double) propbank_role_eval.model),
	   100. * propbank_role_eval.matched / ((double) propbank_role_eval.gold),
	   100. * 2 * propbank_role_eval.matched / ((double) propbank_role_eval.gold + propbank_role_eval.model),
	   100. * nombank_role_eval.matched / ((double) nombank_role_eval.model),
	   100. * nombank_role_eval.matched / ((double) nombank_role_eval.gold),
	   100. * 2 * nombank_role_eval.matched / ((double) nombank_role_eval.gold + nombank_role_eval.model));
    for (i = 0; i < 1000; i++)
      printf(".");  // try to flush output redirection
    printf("\n");
    fflush(stdout);
    fclose(ifp);
    fclose(ofp);
    remove_stata(p);
    return overall_score;
}

double training_epoch(APPROX_PARAMS *p, MODEL_PARAMS *w) {
    char buffer[MAX_LINE];

    SENTENCE *tr_sent;
    double train_err = 0.;
    int c = 0;
    int words_read = 0;
    time_t time_state = time(NULL);


    if (is_blind_file(p->train_file)) {
        fprintf(stderr, "Error: training file does not seem to contain labels (or first line is blank), stopping\n");
        exit(EXIT_FAILURE);
    }

    DEF_FOPEN(ifp, p->train_file, "r");
    while ((tr_sent = read_sentence(w, ifp, 1)) != NULL && words_read < p->train_dataset_size) {
        sprintf(buffer, "%2d: Computing sentence %d (%d words processed), average err: /sent = %.4f, /word = %.4f\n", p->st.loops_num + 1, c, words_read, train_err / c, train_err / words_read);
        save_stata(p, buffer);

        words_read += tr_sent->len;
        OPTION_LIST *der_first, *der_last;
        get_derivation_list(p, w, tr_sent, &der_first, &der_last, 1);
        double train_err_del = -comp_sentence(p, w, der_first);
        train_err += train_err_del;
        comp_sentence_del(p, w, der_last);
	    update_weights(p, w, c);
        free_derivation(der_last);
        free(tr_sent);
//        if  (c != 0 && c % 1 == 0) {
#if PROGRESSDOT
            printf(".");
            fflush(stdout);
#endif
//        }
//      printf("Sentence %d is processed\n", c);

        c++;
    }
    time_t time_end = time(NULL);
    printf("Read %d words, %f s. per sentence.\n", words_read, ((double) (time_end - time_state)) / c);
    fclose(ifp);
    remove_stata(p);
    return train_err;
}

void validation_training(APPROX_PARAMS *ap, MODEL_PARAMS *mp) {


    sample_weights(ap, mp);

    char weights_fname[MAX_NAME];
    strcpy(weights_fname, ap->model_name);
    strcat(weights_fname, ".new.wgt");

    char weight_dels_fname[MAX_NAME];
    strcpy(weight_dels_fname, ap->model_name);
    strcat(weight_dels_fname, ".new.wgt.mom");


    char best_weights_fname[MAX_NAME];
    strcpy(best_weights_fname, ap->model_name);
    strcat(best_weights_fname, ".best.wgt");

    char best_weight_dels_fname[MAX_NAME];
    strcpy(best_weight_dels_fname, ap->model_name);
    strcat(best_weight_dels_fname, ".best.wgt.mom");

    if (is_blind_file(ap->test_file)) {
        fprintf(stderr, "Validation file does not seem to contain gold standard dependency structure (or first line empty)\n");
        fprintf(stderr, "Stopped.");
        exit(EXIT_FAILURE);
    }

    char s[MAX_LINE *25] = "";
    if (load_state(ap)) {
        print_state(s, ap);
        printf("Resuming training with the following parameters:\n====\n%s====\n", s);
        if (strcmp(ap->st.where, PL_FINISHED) == 0) {
            printf("Error: Training had already been finished. WHERE in the state file was set to '%s'\n", PL_FINISHED);
            exit(EXIT_FAILURE);
        }
        if (strcmp(ap->st.where, PL_STARTED) != 0) {
            load_weights(ap, mp, weights_fname);
            load_weight_dels(ap, mp, weight_dels_fname);
        }
    } else {
        ap->st.curr_eta  = ap->init_eta;
        ap->st.curr_reg  = ap->init_reg;
        ap->st.last_tr_err = MAXDOUBLE;
        ap->st.last_score = -MAXDOUBLE;
        ap->st.max_score =  -MAXDOUBLE;
        ap->st.loops_num = 0;
        ap->st.num_sub_opt = 0;
        strcpy(ap->st.where, PL_STARTED);
    }
    ap->st.decay = 1. - ap->st.curr_reg * ap->st.curr_eta;
    save_state(ap);

    ASSERT((ap->st.loops_num == 0)  == (strcmp(ap->st.where, PL_STARTED) == 0));

    double new_tr_err, new_score;
    while (ap->st.loops_num < ap->max_loops) {
        if (ap->st.loops_num > 0 && ap->st.loops_num % ap->loops_between_val == 0 && strcmp(ap->st.where, PL_TRAIN_LOOP_END) == 0) {
            new_score = testing_epoch(ap, mp);
            printf("%2d: [ValTrain] Validation acc :  %f\%%\n", ap->st.loops_num, new_score * 100.);
            if (new_score < ap->st.max_score) {
                ap->st.num_sub_opt++;
            } else {
                printf("%2d: [ValTrain] Best accuracy improved! Saving best weights to %s.\n", ap->st.loops_num, best_weights_fname);
                save_weights(ap, mp, best_weights_fname);
                save_weight_dels(ap, mp, best_weight_dels_fname);
                ap->st.num_sub_opt = 0;
                ap->st.max_score = new_score;
            }
            if (new_score < ap->st.last_score) {
                printf("%2d: [ValTrain] Accuracy decreased.\n", ap->st.loops_num);
                if (ap->eta_red_rate * ap->st.curr_eta / ap->init_eta >= ap->max_eta_red) {
                    ap->st.curr_eta *= ap->eta_red_rate;
                    printf("%2d: [ValTrain] New eta = %e\n", ap->st.loops_num, ap->st.curr_eta);
                }
                if (ap->reg_red_rate * ap->st.curr_reg / ap->init_reg >= ap->max_reg_red) {
                    ap->st.curr_reg *= ap->reg_red_rate;
                    printf("%2d: [ValTrain] New reg = %e\n", ap->st.loops_num, ap->st.curr_reg);
                }
                ap->st.decay = 1. - ap->st.curr_reg * ap->st.curr_eta;
                if (ap->st.num_sub_opt >= ap->max_loops_wo_accur_impr) {
                    printf("%2d: [ValTrain] Too many steps without improvement. Stopping.\n", ap->st.loops_num);
                    break;
                }
            }
            ap->st.last_score = new_score;
            strcpy(ap->st.where, PL_VAL_END);
            save_state(ap);
        }
        ASSERT(strcmp(ap->st.where, PL_STARTED) || strcmp(ap->st.where, PL_VAL_END));
        new_tr_err = training_epoch(ap, mp);
        printf("%2d: [ValTrain] Training error: %f\n", ap->st.loops_num, new_tr_err);
        save_weights(ap, mp, weights_fname);
        save_weight_dels(ap, mp, weight_dels_fname);
        if (ap->st.last_tr_err < new_tr_err) {
            printf("%2d: [ValTrain] Error increase\n", ap->st.loops_num);
            if (ap->eta_red_rate * ap->st.curr_eta / ap->init_eta >= ap->max_eta_red) {
                ap->st.curr_eta *= ap->eta_red_rate;
                printf("%2d: [ValTrain] New eta = %e\n", ap->st.loops_num, ap->st.curr_eta);
            }
            ap->st.decay = 1. - ap->st.curr_reg * ap->st.curr_eta;

        }
        ap->st.last_tr_err = new_tr_err;
        strcpy(ap->st.where, PL_TRAIN_LOOP_END);
        ap->st.loops_num++;
        save_state(ap);
	fflush(stdout);
    }
    printf("%2d: [ValTrain] Finished with the best acurracy: %f\%%\n", ap->st.loops_num, ap->st.max_score * 100);

    strcpy(ap->st.where, PL_FINISHED);
    save_state(ap);
    fflush(stdout);

}
