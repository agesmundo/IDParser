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

#define UPDATE_FREQ 10

double sigmoid(double x) {
    return 1. / (1 + exp(-x));
}

double compute_l(OPTION *opt, int i) {
    return log(opt->previous_option->int_layer.mju[i]) - log(1. - opt->previous_option->int_layer.mju[i]);
}

double compute_l_prime(OPTION *opt, int i) {
    int selected_out = opt->previous_output;
    ASSERT(selected_out >= 0);
        
    return opt->previous_option->out_link->w[selected_out][i];
}

void compute_ab_arrs(OPTION *opt,  int i) {

    double *mju = (opt->new_state) ? opt->prev_layer.mju : opt->int_layer.mju;
    int out;

    //value which is going to be updated
    double f_0 = mju[i]; 

    OPTION *curr_opt = opt->previous_option;
    opt = NULL; //defense 
        
    if (curr_opt->outputs.is_masked) {
        for (out = 0; out < curr_opt->outputs.num; out++) {
            if (!curr_opt->outputs.mask[out]) {
                curr_opt->outputs.b[out] = 0;
                curr_opt->outputs.a[out] = 0;
            } else {
                curr_opt->outputs.b[out] = curr_opt->out_link->w[out][i];
                curr_opt->outputs.a[out] = curr_opt->outputs.parts[out] - curr_opt->out_link->w[out][i] * f_0;
            }
        }
    } else {
        for (out = 0; out < curr_opt->outputs.num; out++) {
            curr_opt->outputs.b[out] = curr_opt->out_link->w[out][i];
            curr_opt->outputs.a[out] = curr_opt->outputs.parts[out] - curr_opt->out_link->w[out][i] * f_0;
        }
    }
}
/* computes the current value of bound part on mju step, assuming  a, b are all already computed */
double _comp_mju_target(double l_sum, OPTION *opt, double p, int rho_idx) {
    int i;
    double v = 0.;
    if (p != 0. && p != 1.) {
        v = - p * log(p) - (1 - p) * log(1 - p);
    }
    v += l_sum * p;

    opt = opt->previous_option;

    opt->outputs.norm = 0.;
    OUTPUTS *outs = &(opt->outputs);
    outs->norm = 0;
    if (opt->outputs.is_masked) {
        for (i = 0; i < opt->outputs.num; i++) {
            if (!opt->outputs.mask[i]) {
                outs->exp[i] = 0;
            } else {
                outs->exp[i] = exp(outs->a[i] + outs->b[i] * p);
                outs->norm +=  outs->exp[i];
            }
        }
    } else {
        for (i = 0; i < opt->outputs.num; i++) {
            outs->exp[i] = exp(outs->a[i] + outs->b[i] * p);
            outs->norm +=  outs->exp[i];
        }
    }
    ASSERT(outs->norm > 0);
    v -= log(outs->norm);
    return v;
} 
void update_output_parts(OPTION *opt, int i, double optimum) {

    opt = opt->previous_option;
    int out;

    OUTPUTS *outs = &(opt->outputs);
    outs->norm = 0;
    if (opt->outputs.is_masked) {
        for (out = 0; out < opt->outputs.num; out++) {
            if (!opt->outputs.mask[out]) {
                outs->parts[out] = - MAXDOUBLE;
                outs->exp[out] = 0.;
            } else {
                outs->parts[out] = outs->a[out] +  outs->b[out] * optimum;
                outs->exp[out] = exp(outs->parts[out]);
                outs->norm += outs->exp[out];
            }
        }
    } else {
        for (out = 0; out < opt->outputs.num; out++) {
            outs->parts[out] = outs->a[out] +  outs->b[out] * optimum;
            outs->exp[out] = exp(outs->parts[out]);
            outs->norm += outs->exp[out];
        }
    }

}

void make_mju_step(OPTION *opt, int rho_idx) { 
    double *mju = (opt->new_state) ? opt->prev_layer.mju : opt->int_layer.mju;

    double l_sum = compute_l(opt, rho_idx) + compute_l_prime(opt, rho_idx);

    //computed and saved in outputs the preprocessed A, B  from exp(A * mju[rho_idx] + B)
    compute_ab_arrs(opt, rho_idx);

    //do golden ratio search (r approx = 0.62)
    double r = (sqrt(5) - 1) / 2.;

    double old_value =  - _comp_mju_target(l_sum, opt, mju[rho_idx], rho_idx);

    double pnts[4], values[4];

    pnts[0] = 0.; 
    pnts[1] = 1 - r;
    pnts[2] = r;
    pnts[3] = 1.;


    int idx; 
    for (idx = 0; idx < 4; idx++) {
        values[idx] = - _comp_mju_target(l_sum, opt, pnts[idx], rho_idx);
    }

    idx = 0; 
    while (fabs(values[1] - values[2]) > MJU_STEP_EPS) {
        if (values[1] < values[2]) {
            pnts[3] = pnts[2];
            values[3] = values[2];

            pnts[2] = pnts[1];
            values[2] = values[1];

            pnts[1] = r * pnts[0] + (1 - r) * pnts[3];
            values[1] =  - _comp_mju_target(l_sum, opt, pnts[1], rho_idx);

        } else {

            pnts[0] = pnts[1];
            values[0] = values[1];

            pnts[1] = pnts[2];
            values[1] = values[2];

            pnts[2] = (1 - r) * pnts[0] + r * pnts[3];
            values[2] = - _comp_mju_target(l_sum, opt, pnts[2], rho_idx);
        }
        idx++;
    }

    if (values[1] < old_value || values[2] < old_value) {
        if (values[1] < values[2]) {
            update_output_parts(opt, rho_idx,  pnts[1]); 
            mju[rho_idx] = pnts[1];
        } else {
            update_output_parts(opt, rho_idx, pnts[2]);  
            mju[rho_idx] = pnts[2];
        }
    } else {
        //do nothing
    }
}


void comp_output_parts(APPROX_PARAMS *ap, OPTION *opt, double *mju) {

    ASSERT(opt->outputs.num >= 0);
    
    int out, i;

    opt->outputs.norm = 0;
    if (opt->outputs.is_masked) {
        for (out = 0; out < opt->outputs.num; out++) {
            if (!opt->outputs.mask[out]) {
                opt->outputs.parts[out] = - MAXDOUBLE;
                opt->outputs.exp[out] = 0;
                continue;
            }
            opt->outputs.parts[out] = opt->out_link->b[out];
            for (i = 0; i < ap->hid_size; i++) {
                opt->outputs.parts[out]  +=  opt->out_link->w[out][i] * mju[i];
            }
            opt->outputs.exp[out] = exp(opt->outputs.parts[out]);
            opt->outputs.norm += opt->outputs.exp[out];
        }
        
    } else {
        for (out = 0; out < opt->outputs.num; out++) {
            opt->outputs.parts[out] = opt->out_link->b[out];
            for (i = 0; i < ap->hid_size; i++) {
                opt->outputs.parts[out]  +=  opt->out_link->w[out][i] * mju[i];
            }
            opt->outputs.exp[out] = exp(opt->outputs.parts[out]);
            opt->outputs.norm += opt->outputs.exp[out];
        }
    }
}


double comp_slice_target_and_parts(APPROX_PARAMS *ap, OPTION *opt) {
    double lh = 0.;
    int i;

    ASSERT(opt->previous_option != NULL);

    double *mju;
    if (opt->new_state) {
        //we are computing update after previous timeslot
        mju = opt->prev_layer.mju;
    } else {
        //word prediction - we are computing update after previous tag prediction
        mju = opt->int_layer.mju;
    }

    double *prev_opt_mju = opt->previous_option->int_layer.mju;

    // comp_hid_fw_means_vars(opt);
    for (i = 0; i < ap->hid_size; i++) {
        ASSERT(mju[i] != 0. && mju[i] != 1.);
        if (mju[i] != 0. && mju[i] != 1.) {
            lh -= mju[i] * log(mju[i]) 
                + (1 - mju[i]) * log(1 - mju[i]);
        }

        lh += mju[i]* (log(prev_opt_mju[i]) - log(1 - prev_opt_mju[i]));
        lh += log(1 - prev_opt_mju[i]); ///not needed, actually...
    }

    OPTION *out_opt = opt->previous_option;
    int selected_out = opt->previous_output;
    comp_output_parts(ap, out_opt, mju);
    lh += out_opt->outputs.parts[selected_out];
    lh -= log(out_opt->outputs.norm);

    return lh; 
}


void fill_lprob(OPTION *opt) {
    if (opt->previous_option != NULL) {
        OPTION *prev_opt = opt->previous_option;
        opt->lprob = prev_opt->lprob + log(prev_opt->outputs.q[opt->previous_output]);
    } else {
        opt->lprob = 0.;
    }
}

void comp_hid_fw_means(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt) {
    ASSERT(opt->new_state);

    int i, j, l;
    
    for (i = 0; i < ap->hid_size; i++) {
        opt->fw_means[i] = 0;
        for (l = 0;  l < opt->inp_num; l++) {
            opt->fw_means[i] += opt->inp_biases[l]->b[i];
        }
        for (l = 0; l < opt->rel_num; l++) {
            for (j = 0; j < ap->hid_size; j++) {
                opt->fw_means[i] += opt->rel_weights[l]->w[i][j] * opt->rel_layers[l]->mju[j];
            }
        }
    }
}

int had_choice(OPTION *opt) {
    if (opt->outputs.is_masked) {
        int out;
        int allowed = 0;
        for (out = 0; out < opt->outputs.num; out++) {
            if (opt->outputs.mask[out]) {
                allowed++;
            }
        }
        ASSERT(allowed > 0);
        return allowed > 1;
    } else {
        return (opt->outputs.num > 1);
    }
}

void comp_option_states(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt)
{
    int i;
    if (ap->approx_type == FF) {
        if (opt->new_state) {
            if (opt->previous_option != NULL) {
                memcpy(&opt->prev_layer, &opt->previous_option->int_layer, sizeof(LAYER));
            }
            //make forward computations and fill int_layer.mju
            comp_hid_fw_means(ap, w, opt);
            //forward initialization
            for (i = 0; i < ap->hid_size; i++) {
                opt->int_layer.mju[i] = sigmoid(opt->fw_means[i]);
            }
        } else {
            //just copy from a previous option in the same timeslot
            memcpy(&opt->int_layer, &opt->previous_option->int_layer, sizeof(LAYER));
        }

    } else {
        ASSERT(ap->approx_type == MF);
        if (opt->new_state) {
            if (opt->previous_option != NULL) {
                //copy from a previous option
                memcpy(&opt->prev_layer, &opt->previous_option->int_layer, sizeof(LAYER));

                if (had_choice(opt->previous_option)) {
                    double curr_slice_bound = comp_slice_target_and_parts(ap, opt); 
                    double prev_slice_bound = - MAXDOUBLE;        
                    int loop = 0;
                    while (curr_slice_bound - prev_slice_bound > SLICE_ALT_EPS) {
                        for (i = 0; i < ap->hid_size; i++) {
                            make_mju_step(opt, i);
                        }
                        prev_slice_bound = curr_slice_bound;
                        curr_slice_bound = comp_slice_target_and_parts(ap, opt);
                        loop++;
                        //printf("In loop (1): curr_slice_bound = %f, prev_slice_bound = %f\n", curr_slice_bound, prev_slice_bound);
                    }
                    ASSERT(curr_slice_bound >= prev_slice_bound);
                    
                    //DEBUG
                    //if (!(curr_slice_bound >= prev_slice_bound)) {
                    //    printf("After loop (2): curr_slice_bound = %f, prev_slice_bound = %f\n", curr_slice_bound, prev_slice_bound);
                    //}
                }
            }

            //make forward computations and fill int_layer.mju
            comp_hid_fw_means(ap, w, opt);
            //forward initialization
            for (i = 0; i < ap->hid_size; i++) {
                opt->int_layer.mju[i] = sigmoid(opt->fw_means[i]);
            }

            // !opt->new_state   
        } else {
            memcpy(&opt->int_layer, &opt->previous_option->int_layer, sizeof(LAYER));
            if (had_choice(opt->previous_option)) {
                double curr_slice_bound = comp_slice_target_and_parts(ap, opt); 
                double prev_slice_bound = - MAXDOUBLE;        
                int loop = 0;
                while (curr_slice_bound - prev_slice_bound > SLICE_ALT_EPS) {
                    for (i = 0; i < ap->hid_size; i++) {
                        make_mju_step(opt, i);
                    }
                    prev_slice_bound = curr_slice_bound;
                    curr_slice_bound = comp_slice_target_and_parts(ap, opt);
                    loop++;
                  
                    //printf("In loop (2): curr_slice_bound = %f, prev_slice_bound = %f (loop = %d)\n", curr_slice_bound, prev_slice_bound, loop);
                }
                ASSERT(curr_slice_bound >= prev_slice_bound);
                
                //DEBUG
                if (!(curr_slice_bound >= prev_slice_bound)) {
                    printf("After loop (2): curr_slice_bound = %f, prev_slice_bound = %f\n", curr_slice_bound, prev_slice_bound);
                }
           }
        } // !opt->new_state

    } //MF

}
//norm and exp  fields of outputs should be precomputed
void comp_output_distr_from_parts(OPTION *opt) {
    int out;
    for (out = 0; out < opt->outputs.num; out++) {
        opt->outputs.q[out] = opt->outputs.exp[out] / opt->outputs.norm;
    }
}

void comp_option_outputs(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt) {
    comp_output_parts(ap, opt,  opt->int_layer.mju);
    comp_output_distr_from_parts(opt);
}

void comp_option(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION *opt) {
    if (opt->outputs.mask == NULL)
      alloc_option_outputs(opt);
    fill_lprob(opt);
    comp_option_states(ap, w, opt);
    comp_option_outputs(ap, w, opt);
}

double comp_sentence(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *head) {
    ASSERT(ap->approx_type == FF || ap->approx_type == MF);
    OPTION_LIST *curr_node = head;
    OPTION *curr_opt = NULL;

    while(curr_node->next->next != NULL) {
        curr_opt = curr_node->elem;
        fill_lprob(curr_opt);
        comp_option_states(ap, w, curr_opt); 
        comp_option_outputs(ap, w, curr_opt);
        
        curr_node = curr_node->next;
    }

    curr_opt = curr_node->elem;
    fill_lprob(curr_opt);

    //IMPORTANT!!!!!!!!!!!!!!!!!!!!!
    ///THERE IS A PROBLEM WITH ERROR COMPS: PRECOMPUTED EXPONENTS, PARTS AND NORM ARE FILLED WITH COMPUTATIONS
    // FROM NEXT TIME SLOT (in the mean time q - is correct)
    return curr_opt->lprob;
}


void comp_out_local_err(APPROX_PARAMS *ap, OPTION *opt, int corr_out) {
    
    double *mju = opt->int_layer.mju;
    double *mju_err = opt->int_layer.mju_err;

    int out;
    for (out = 0; out < opt->outputs.num; out++) {
        opt->outputs.del[out] = - opt->outputs.q[out];
    }
    opt->outputs.del[corr_out] += 1.;

    for (out = 0; out < opt->outputs.num; out++) {
        opt->out_link->b_del[out] += opt->outputs.del[out];
        int j;
        for (j = 0; j < ap->hid_size; j++) {
            opt->out_link->w_del[out][j] += opt->outputs.del[out] * mju[j];
            mju_err[j] += opt->outputs.del[out] * opt->out_link->w[out][j];
        }
    }
}

//compute weight updates due to global error, 
//first_opt is first option in slice
//mju  state values for which error is computed
//mju_err error for this state to distribute to links
void comp_hid_linkerr_anal(APPROX_PARAMS *ap, OPTION *first_opt, double *mju, double *mju_err) {
    ASSERT(first_opt->new_state);

    int i, j, l;
    for (i = 0; i < ap->hid_size; i++) {
        double const_del = mju_err[i] * mju[i] * (1 - mju[i]);


        for (l = 0;  l < first_opt->inp_num; l++) {
            first_opt->inp_biases[l]->b_del[i] += const_del;
        }
        for (l = 0; l < first_opt->rel_num; l++) {
            double *w_del = first_opt->rel_weights[l]->w_del[i];
            double *rel_mju = first_opt->rel_layers[l]->mju;
            for (j = 0; j < ap->hid_size; j++) {
                w_del[j] += const_del * rel_mju[j];
            }
        }
    } //for i         
}

//compute prev mju updates due to global error, 
//first_opt is first option in slice
//mju  state values for which error is computed
//mju_err error for this state to distribute to links
void comp_hid_mjuerr_anal(APPROX_PARAMS *ap, OPTION *first_opt, double *mju, double *mju_err) {
    ASSERT(first_opt->new_state);
    int i, j, l;

    for (l = 0; l < first_opt->rel_num; l++) {
        double *rel_mju_err = first_opt->rel_layers[l]->mju_err;
        HH_LINK *rel_weight = first_opt->rel_weights[l];
        for (j = 0; j < ap->hid_size; j++) {
            double del_j = mju_err[j] *  mju[j] * (1 - mju[j]);
            double *rel_weight_j = rel_weight->w[j];
            for (i = 0; i < ap->hid_size; i++) {
                rel_mju_err[i] += del_j * rel_weight_j[i];
            }
        }        
    }
    
}


//computes - dF_i/d_mju_s matrix 
//expr parts should be computed before hand
void comp_der_matrix(APPROX_PARAMS  *ap, OPTION *opt, double *mju, double *m) {

    //i - row num 
    int i;
    for (i = 0; i < ap->hid_size; i++) {

        //////////////////////////////////////
        //m[i][i] computation
        /////////////////////////////////////
        m[i * ap->hid_size + i] = + 1. / (mju[i] * (1 - mju[i]));

        OPTION *out_opt = opt->previous_option;
        OUTPUTS *outputs = &(out_opt->outputs);

        //computes expectations
        int out;
        double nom1 = 0., nom2 = 0., denom = 0.;
        for (out = 0; out < outputs->num; out++) {
            if (outputs->is_masked && outputs->mask[out] == 0) {
                continue;
            }
            double exp_val = outputs->exp[out];
            double w_i = out_opt->out_link->w[out][i];
            nom1 += exp_val * SQR(w_i);
            nom2 += exp_val * w_i;
            denom += exp_val;
        }
        m[i * ap->hid_size + i]  -= - nom1 / denom  + SQR(nom2 / denom);
        ////////////////////////////////////
        //end of m[i][i] computation
        ///////////////////////////////////         


        //column num
        int s;
        for (s = i + 1; s < ap->hid_size; s++) {
            m[i * ap->hid_size + s] = 0.;

            OPTION *out_opt = opt->previous_option;
            OUTPUTS *outputs = &(out_opt->outputs);
            double nom1 = 0., nom2_1 = 0., nom2_2 = 0., denom = 0.;

            //computs exps 
            int out;    
            for (out = 0; out < outputs->num; out++) {
                if (outputs->is_masked && outputs->mask[out] == 0) {
                    continue;
                }
                double exp_val = outputs->exp[out];
                double w_i = out_opt->out_link->w[out][i];
                double w_s = out_opt->out_link->w[out][s];

                nom1 += exp_val * w_i * w_s;
                nom2_1 += exp_val * w_i;
                nom2_2 += exp_val * w_s;
                denom += exp_val;
            }


            m[i * ap->hid_size + s]  -= - nom1 / denom  + nom2_1 * nom2_2 / SQR(denom);
            m[s * ap->hid_size + i] = m[i * ap->hid_size + s];            

        } //for s
    } // for i
}

    
void comp_mju_mju_seq_err(APPROX_PARAMS *ap, OPTION *opt, gsl_matrix_view  *der_f_mju, gsl_permutation *permut, double *incom_err) {
    int i, k; 
    ASSERT(opt->previous_option != NULL)
    double der_f_w[MAX_HID_SIZE];

    double *rel_state_err = opt->previous_option->int_layer.mju_err;
    double *rel_state_act = opt->previous_option->int_layer.mju;

    gsl_vector *der_mju_w_vector = gsl_vector_alloc(ap->hid_size);
    CHECK_PTR(der_mju_w_vector); 

    for (i = 0; i < ap->hid_size; i++) {
        bzero(der_f_w, sizeof(double) *  ap->hid_size);
        der_f_w[i] = 1. / (rel_state_act[i] * (1 - rel_state_act[i]));
        gsl_vector_view b  = gsl_vector_view_array (der_f_w, ap->hid_size);
        int status = gsl_linalg_LU_solve (&(*der_f_mju).matrix, permut, &b.vector, der_mju_w_vector);
        if (status) {
            printf ("error: %s\n", gsl_strerror (status));
            fprintf(stderr, "GSL library error\n");
            exit(1);
        }
        for (k = 0; k < ap->hid_size; k++) {
            rel_state_err[i] += incom_err[k] * der_mju_w_vector->data[k * der_mju_w_vector->stride];
        }
    } //for i         
    gsl_vector_free(der_mju_w_vector);
}


void comp_out_global_err(APPROX_PARAMS *ap,  OPTION *after_opt, gsl_matrix_view  *der_f_mju, 
        gsl_permutation *permut, double *mju, double *mju_err) {

    double m_w[MAX_HID_SIZE];
    double err_j_coeff[MAX_HID_SIZE];
    double single_1[MAX_HID_SIZE];
    double der_f_w[MAX_HID_SIZE];
    gsl_vector *inverse_cols_vector = gsl_vector_alloc(ap->hid_size);
    CHECK_PTR(inverse_cols_vector); 
    gsl_vector *der_mju_w_vector = gsl_vector_alloc(ap->hid_size);
    CHECK_PTR(der_mju_w_vector); 

    OPTION *opt = after_opt->previous_option;
    int selected_out = after_opt->previous_output;

    int out, j, k;
    double denom = 0.;
    bzero(m_w, ap->hid_size * sizeof(double));

    //comput w[.][k] expectation over outputs
    for (out = 0; out < opt->outputs.num; out++) {
        if (opt->outputs.is_masked && opt->outputs.mask[out] == 0) {
            continue;
        }
        double exp_val = opt->outputs.exp[out];
        denom += exp_val;
        for (k = 0; k < ap->hid_size; k++) {
            double w_k = opt->out_link->w[out][k];
            m_w[k] += exp_val * w_k;
        }
    } 
    for (k = 0; k < ap->hid_size; k++) {
        m_w[k] /= denom;
    }

    //err_j_coeff - component use in the error part dependent on j
    bzero(err_j_coeff, ap->hid_size * sizeof(double));        

    //vector in which only single component will be set to zero (to find row of inverse matrix)
    bzero(single_1, ap->hid_size * sizeof(double));

    for (j = 0; j < ap->hid_size; j++) {
        single_1[j] = 1.;

        gsl_vector_view b  = gsl_vector_view_array (single_1, ap->hid_size);
        int status = gsl_linalg_LU_solve (&(*der_f_mju).matrix, permut, &b.vector, inverse_cols_vector);
        if (status) {
            printf ("error: %s\n", gsl_strerror (status));
            fprintf(stderr, "GSL library error\n");
            exit(1);
        }
        for (k = 0; k < ap->hid_size; k++) {
            err_j_coeff[j] += mju_err[k] * inverse_cols_vector->data[k * inverse_cols_vector->stride];
        }
        single_1[j] = 0.;
    }

    for (out = 0; out <  opt->outputs.num; out++) {
        //DEBUG
        if (opt->outputs.is_masked && opt->outputs.mask[out] == 0) {
            continue;
        }
        
        //by we  new w we computed err_j_coeff which will be multiplied by delta(y^0, t) - p_t to get dependent component of 
        //dL / d(w_(tj))
        //now let us compute part which is independent of j

        double p = opt->outputs.exp[out] / denom;
        //it is not really der_f_w - it smth in side which we are going to multiply by  mju_j later
        for (k = 0; k < ap->hid_size; k++) {
            der_f_w[k] = -p * (opt->out_link->w[out][k] - m_w[k]);
        }
        gsl_vector_view b  = gsl_vector_view_array (der_f_w, ap->hid_size);
        int status = gsl_linalg_LU_solve (&(*der_f_mju).matrix, permut, &b.vector, der_mju_w_vector);
        if (status) {
            printf ("error: %s\n", gsl_strerror (status));
            fprintf(stderr, "GSL library error\n");
            exit(1);
        }

        //independent error component
        double ind_err = 0.;
        for (k = 0; k < ap->hid_size; k++) {
            ind_err += mju_err[k] * der_mju_w_vector->data[k * der_mju_w_vector->stride];
        }

        //now we are ready to compute the actual error dL/dw_{tj}
        if (selected_out != out) {
            for (j = 0; j < ap->hid_size; j++) {
                opt->out_link->w_del[out][j] +=  ind_err * mju[j]  + (-p) * err_j_coeff[j];
            }
        } else {
            for (j = 0; j < ap->hid_size; j++) {
                opt->out_link->w_del[out][j] +=  ind_err * mju[j]  + (1. -p) * err_j_coeff[j];
            }
        } 
        opt->out_link->b_del[out] += ind_err;
        out++;
    }
    gsl_vector_free(inverse_cols_vector);
    gsl_vector_free(der_mju_w_vector);
}


void update_weights(APPROX_PARAMS *ap, MODEL_PARAMS *mp, int sent)
{

    if (sent % UPDATE_FREQ != 0)
        return;

    double eta = ap->st.curr_eta;
    double mom = ap->mom;
    double dec = ap->st.decay;
    {
      int i;
      for (i = 1; i < UPDATE_FREQ; i++) {
	mom *= ap->mom;
      }
    }
#define UPDATE_IH_WEIGHTS(ih)\
    {\
        for (h = 0; h < ap->hid_size; h++) { \
            (ih).b[h] += eta * (ih).b_del[h];\
            (ih).b[h] *= dec; \
            (ih).b_del[h] *= mom;\
        }\
    } 
    
    //save input links
    int l;
    for (l = 0; l < mp->ih_links_num; l++) {
        if (mp->ih_links_specs[l].info_type == CPOS_TYPE) {
            int i, h;
            //+ 1  to represent the fact that there is
            //no word complying to the specification l
            for (i = 0; i < mp->cpos_num + 1; i++) {
                UPDATE_IH_WEIGHTS(mp->ih_link_cpos[l][i]);
            }
        } else if (mp->ih_links_specs[l].info_type == DEPREL_TYPE) {
            int i,h;
            // +2 : for ROOT relationship and to represent fact that there is
            // not word which complies with the specification l
            for (i = 0; i < mp->deprel_num + 2; i++) {
                   UPDATE_IH_WEIGHTS(mp->ih_link_deprel[l][i]);
            }   
        } else if (mp->ih_links_specs[l].info_type == POS_TYPE) {
            int i,h;
            for (i = 0; i < mp->pos_info_in.num + 1; i++) {
                UPDATE_IH_WEIGHTS(mp->ih_link_pos[l][i]);
            }   
        } else if (mp->ih_links_specs[l].info_type == LEMMA_TYPE) {
            int i,h;
            for (i = 0; i < mp->lemma_num + 1; i++) {
                UPDATE_IH_WEIGHTS(mp->ih_link_lemma[l][i]);
            }   
        } else if (mp->ih_links_specs[l].info_type == FEAT_TYPE) {
            if (ap->inp_feat_mode == FEAT_MODE_COMPOSED || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int p, f, h;
                for (p = 0; p < mp->pos_info_in.num; p++) {
                    for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                        UPDATE_IH_WEIGHTS(mp->ih_link_feat[l][p][f]);
                    }
                }   
                //link for no word
                UPDATE_IH_WEIGHTS(mp->ih_link_feat[l][mp->pos_info_in.num][0]);
            }
            if (ap->inp_feat_mode == FEAT_MODE_ELEMENTARY || ap->inp_feat_mode == FEAT_MODE_BOTH) {
                int i,h;
                for (i = 0; i < mp->elfeat_num + 1; i++) {
                    UPDATE_IH_WEIGHTS(mp->ih_link_elfeat[l][i]);
                }
            }
        } else if (mp->ih_links_specs[l].info_type == SENSE_TYPE) {
            int b, lemma, s, h;
            for (b = 0; b < BANK_NUM; b++) {
                for (lemma = 0; lemma < mp->lemma_num; lemma++) {
                    for (s = 0; s < mp->sense_num[b][lemma]; s++) {
                        UPDATE_IH_WEIGHTS(mp->ih_link_sense[l][b][lemma][s]);
                    }
                }
            }
            UPDATE_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM][0][0]);
            UPDATE_IH_WEIGHTS(mp->ih_link_sense[l][BANK_NUM + 1][0][0]);
      } else if (mp->ih_links_specs[l].info_type ==  WORD_TYPE) {
            int p, f, w, h;
            for (p = 0; p < mp->pos_info_in.num; p++) {
                for (f = 0; f < mp->pos_info_in.feat_infos[p].num; f++) {
                    for (w = 0; w < mp->pos_info_in.feat_infos[p].word_infos[f].num; w++) {
                        UPDATE_IH_WEIGHTS(mp->ih_link_word[l][p][f][w]);
                    }
                }
            }   
            //link for no word
            UPDATE_IH_WEIGHTS(mp->ih_link_word[l][mp->pos_info_in.num][0][0]);
        }  else if (mp->ih_links_specs[l].info_type ==  SYNT_LAB_PATH_TYPE) {
            int p, h;
            int path_num = get_synt_path_feature_size(mp);
            for (p = 0; p < path_num + 1; p++) {
                UPDATE_IH_WEIGHTS(mp->ih_link_synt_lab_path[l][p]);
            }
        } else {
            ASSERT(0);
        }
    }

    int h;
    UPDATE_IH_WEIGHTS(mp->init_bias);
    UPDATE_IH_WEIGHTS(mp->hid_bias);
    UPDATE_IH_WEIGHTS(mp->hid_bias_srl);

    int a;
    for (a = 0; a < ACTION_NUM; a++) {
        UPDATE_IH_WEIGHTS(mp->prev_act_bias[a]);
    }

    for (a = 0; a < ACTION_NUM; a++) {
        int d;
        for (d = 0; d < mp->deprel_num; d++) {
            UPDATE_IH_WEIGHTS(mp->prev_deprel_bias[a][d]);
        }
        int b;
        int max = 1;
        for (b = 0; b < BANK_NUM; b++) {
            if (mp->role_num[b] > max) {
                max = mp->role_num[b];
            }
        }
        for (d = 0; d < max; d++) {
            UPDATE_IH_WEIGHTS(mp->prev_arg_bias[a][d]);
        }

    }        
    


    // HID-HID weight matrixes
    int h1, h2;
    for (l = 0; l < mp->hh_links_num; l++) {
        for (h1 = 0; h1 < ap->hid_size; h1++) {
            for (h2 = 0; h2 < ap->hid_size; h2++) {
                mp->hh_link[l].w[h1][h2] += eta * mp->hh_link[l].w_del[h1][h2];
                mp->hh_link[l].w[h1][h2] *= dec;
                mp->hh_link[l].w_del[h1][h2] *= mom;
            }
        }
    }
        
    
    //output weights
#define UPDATE_OUT_LINK(out_link)\
    {\
        (out_link).b[t] += eta * (out_link).b_del[t];\
        (out_link).b[t] *= dec; \
        (out_link).b_del[t] *= mom; \
        for (i = 0; i < ap->hid_size; i++) { \
            (out_link).w[t][i] += eta * (out_link).w_del[t][i];\
            (out_link).w[t][i] *= dec; \
            (out_link).w_del[t][i]  *= mom; \
        }\
    } 
    
    int t, i;
    for (t = 0; t < ACTION_NUM; t++) {
        UPDATE_OUT_LINK(mp->out_link_act);
    }
    
    for (t = 0; t < mp->deprel_num; t++) {
        UPDATE_OUT_LINK(mp->out_link_la_label);
        UPDATE_OUT_LINK(mp->out_link_ra_label);
    }

    ASSERT(ap->input_offset == 0 || ap->input_offset == 1);
    int pos_num = ap->input_offset + mp->pos_info_out.num;
    for (t = 0; t < pos_num; t++) {
        UPDATE_OUT_LINK(mp->out_link_pos);
    } 

    int p;
    for (p = 0; p < pos_num; p++) {
        for (t = 0; t < mp->pos_info_out.feat_infos[p].num; t++) {
            UPDATE_OUT_LINK(mp->out_link_feat[p]);
        }
    }
    
    int f;
    for (p = 0; p < pos_num; p++) {
        for (f = 0; f < mp->pos_info_out.feat_infos[p].num; f++) {
            for (t = 0; t < mp->pos_info_out.feat_infos[p].word_infos[f].num; t++) {
                UPDATE_OUT_LINK(mp->out_link_word[p][f]);
            }
        }
    }    
     
    int b;
    for (b = 0; b < BANK_NUM; b++) {
        for (t = 0; t < mp->role_num[b]; t++) {
            UPDATE_OUT_LINK(mp->out_link_sem_la_label[b]);
        }
    }
    for (b = 0; b < BANK_NUM; b++) {
        for (t = 0; t < mp->role_num[b]; t++) {
            UPDATE_OUT_LINK(mp->out_link_sem_ra_label[b]);
        }
    }


    for (b = 0; b < BANK_NUM; b++) {
        int l;
        for (l = 0; l < mp->lemma_num; l++) {
            for (t = 0; t <  mp->sense_num[b][l]; t++) {
                UPDATE_OUT_LINK(mp->out_link_sense[b][l][0]);
            }
        }
    }
}


OPTION *get_first_option_in_slot(OPTION *opt) {
    OPTION *curr_opt = opt;
    while (!curr_opt->new_state) {
        curr_opt = curr_opt->previous_option;
    }

    return curr_opt;
}

void comp_sentence_del_ssn(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *der_last) {

    OPTION_LIST  *curr_node = der_last->prev->prev;
    int corr_out = der_last->prev->elem->previous_output;
    ASSERT(corr_out >= 0);

    while (curr_node != NULL) {
        OPTION *opt = curr_node->elem;

        comp_output_parts(ap, opt, opt->int_layer.mju);

        //computes errors in respect of output weights and int_mju (compute int_mju_err)
        comp_out_local_err(ap, opt, corr_out);

        //first opt in slice
        OPTION *first_opt = get_first_option_in_slot(opt);
        //it was analytically computed  - errors in respect to rho-rho weights
        comp_hid_linkerr_anal(ap, first_opt, opt->int_layer.mju, opt->int_layer.mju_err);

        //update error for the previous layers
        comp_hid_mjuerr_anal(ap, first_opt, opt->int_layer.mju, opt->int_layer.mju_err);

        if (opt->new_state && opt->previous_option != NULL) {
            //first opt in the previous slice (in the one we actually consider here)
            OPTION *first_opt = get_first_option_in_slot(opt->previous_option);
            //it was analytically computed  - errors in respect to rho-rho weights
            comp_hid_linkerr_anal(ap, first_opt, opt->prev_layer.mju, opt->prev_layer.mju_err);

            //update error for the previous layers
            comp_hid_mjuerr_anal(ap, first_opt, opt->prev_layer.mju, opt->prev_layer.mju_err);
        }    

        corr_out = opt->previous_output;
        curr_node = curr_node->prev;
    }
}

void comp_sentence_del_mf(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *der_last) {
    //node that points to the last option with a computed state
    OPTION_LIST *curr_node = der_last->prev->prev;
    int corr_out = der_last->prev->elem->previous_output;

    ASSERT(corr_out >= 0);

    DEF_ALLOC_ARRAY(der_f_mju_data, double, ap->hid_size * ap->hid_size);
    gsl_matrix_view der_f_mju;
    gsl_permutation *permut = NULL;
    int sign;

    while (curr_node != NULL) {
        OPTION  *opt = curr_node->elem;

        //prepare exp in current option and in the previous option (if it shares the state vector)
        comp_output_parts(ap, opt, opt->int_layer.mju);
        if (!opt->new_state && opt->previous_option != NULL) {
            comp_output_parts(ap, opt->previous_option, opt->int_layer.mju);
        }

        //computes ilocal errors in respect of output weights and int_mju (compute int_mju_err): dq(corr_out)/dw (or /dmju)
        comp_out_local_err(ap, opt, corr_out);

        if (opt->new_state) 
        {
            //it was analytically computed  - errors in respect to rho-rho weights
            comp_hid_linkerr_anal(ap, opt, opt->int_layer.mju, opt->int_layer.mju_err);

            //update error for the previous layers
            comp_hid_mjuerr_anal(ap, opt, opt->int_layer.mju, opt->int_layer.mju_err);

            //no comp_out_global_err - it does not use any outputs in computation
        } else {

            //compute int_mju derivative matrix 
            if (permut != NULL) {
                gsl_permutation_free(permut);
            }

            comp_der_matrix(ap, opt, opt->int_layer.mju, der_f_mju_data);

            der_f_mju = gsl_matrix_view_array (der_f_mju_data, ap->hid_size, ap->hid_size);
            permut = gsl_permutation_alloc (ap->hid_size);


            int status = gsl_linalg_LU_decomp (&der_f_mju.matrix, permut, &sign);
            if (status) {
                printf ("error: %s\n", gsl_strerror (status));
                fprintf(stderr, "GSL library error\n");
                exit(1);
            }
            comp_mju_mju_seq_err(ap, opt, &der_f_mju, permut, opt->int_layer.mju_err);
            comp_out_global_err(ap,  opt, &der_f_mju, permut, opt->int_layer.mju, opt->int_layer.mju_err);
        }
        //error computaion will be done for the previous slice
        //it is  not important that it isn not done for the last - der in respect to the last final mju slice 
        //are zero (nothing depends on them)
        if (opt->new_state && opt->previous_option != NULL) {

            comp_output_parts(ap, opt->previous_option, opt->prev_layer.mju);

            //compute state int_mju derivative matrix 
            if (permut != NULL) {
                gsl_permutation_free(permut);
            }
            comp_der_matrix(ap, opt, opt->prev_layer.mju, der_f_mju_data);
            der_f_mju = gsl_matrix_view_array (der_f_mju_data, ap->hid_size, ap->hid_size);
            permut = gsl_permutation_alloc (ap->hid_size);

            int status = gsl_linalg_LU_decomp (&der_f_mju.matrix, permut, &sign);
            if (status) {
                printf ("error: %s\n", gsl_strerror (status));
                fprintf(stderr, "GSL library error\n");
                exit(1);
            }

            comp_mju_mju_seq_err(ap, opt, &der_f_mju, permut, opt->prev_layer.mju_err);
            comp_out_global_err(ap,  opt, &der_f_mju, permut, opt->prev_layer.mju, opt->prev_layer.mju_err);
        }  
        corr_out = opt->previous_output;
        curr_node = curr_node->prev;
    }

    free(der_f_mju_data);
    if (permut != NULL) {
        gsl_permutation_free (permut);
    }
} 
void comp_sentence_del(APPROX_PARAMS *ap, MODEL_PARAMS *w, OPTION_LIST *der_last) {
    if (ap->approx_type == MF) {
        comp_sentence_del_mf(ap, w, der_last);
    } else {
        ASSERT(ap->approx_type == FF);
        comp_sentence_del_ssn(ap, w, der_last);
    }
}

