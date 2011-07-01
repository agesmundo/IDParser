#include "idp.h"


//----------------------------  OPERATIONS WITH PART_SRL

PART_SRL *ps_alloc(int sent_len) {
    DEF_ALLOC(ps, PART_SRL);
    ps->len = sent_len;
    ALLOC_ARRAY(ps->nodes, NODE_SRL, sent_len + 1);
    int i;
    for (i = 0; i < sent_len + 1; i++) {
        ps->nodes[i].arg_num = 0;
        ps->nodes[i].head_num = 0;
        ps->nodes[i].sense = -1;
    }
    return ps;
}


PART_SRL *ps_clone(PART_SRL *ps) {
    PART_SRL *psc = ps_alloc(ps->len);
    memcpy(psc->right_connections_num, ps->right_connections_num, sizeof(int) * (ps->len + 1));
    memcpy(psc->nodes, ps->nodes, sizeof(NODE_SRL) * (ps->len + 1)); 
    return psc;
}

void ps_free(PART_SRL **ps) {
    free((*ps)->nodes);
    free(*ps);
    *ps = NULL;
}

int ps_arg(PART_SRL *ps, int i, int role) {
    ASSERT(i <= ps->len);

    int c;
    for (c = 0; c < ps->nodes[i].arg_num; c++) {
        if (ps->nodes[i].arg_roles[c] == role) {
            return ps->nodes[i].args[c];
        }
    }
    return 0;
}

int ps_is_head_of(PART_SRL *ps, int h, int a) {
    int w;
    for (w = 0; w < ps->nodes[a].head_num; w++) {
        if (ps->nodes[a].heads[w] == h) {
            return 1;
        }
    }
    return 0;
}

int is_ps_role(PART_SRL *ps, int h, int a, int r) {
    int w;
    for (w = 0; w < ps->nodes[a].head_num; w++) {
        if (ps->nodes[a].heads[w] == h && ps->nodes[a].my_roles[w] == r) {
	  return 1;
        }
    }
    return 0;
}




// sets that the token is a predicate and sets the sense for the predicate
void ps_set_sense(PART_SRL *ps, int i, int sense) {  
    ASSERT(i <= ps->len);
    ps->nodes[i].sense = sense;
}


void ps_add_link(PART_SRL *ps, int h, int d, int role) {
    ASSERT(d > 0 && h >= 0 && ps->nodes[h].sense >= 0);
    
    ps->nodes[d].heads[ps->nodes[d].head_num] = h;
    ps->nodes[d].my_roles[ps->nodes[d].head_num] = role;
    ps->nodes[d].head_num++;

    //TODO should we have h == 0 (how we denote unattached tokens???)
    if (h > 0) {
        int i;
        for (i = ps->nodes[h].arg_num - 1; i >= 0; i--) {
            int c_d = ps->nodes[h].args[i];
            if (c_d >= d) { // allow multiple of same pair (JH)
                ps->nodes[h].args[i + 1] = c_d;
                ps->nodes[h].arg_roles[i + 1] =  ps->nodes[h].arg_roles[i];
            } else {
                ASSERT(c_d != d);
                break;     
            }
        }
        
        ps->nodes[h].args[i + 1] = d;
        ps->nodes[h].arg_roles[i + 1] = role;
        ps->nodes[h].arg_num++;
    } 
}

void ps_fill_sentence(PART_SRL *ps, MODEL_PARAMS *mp, SENTENCE *sent) {
    ASSERT(ps->len == sent->len);
    int t;
    for (t = 1; t <= sent->len; t++) {
        sent->arg_num[t] = ps->nodes[t].arg_num;
        sent->sense[t] = ps->nodes[t].sense;
        if (ps->nodes[t].sense >= 0) {
            int bank = sent->bank[t];
            ASSERT(bank < BANK_NUM);
            strcpy(sent->s_sense[t], mp->s_sense[bank][sent->lemma[t]][ps->nodes[t].sense]);
            int a;
            for (a = 0; a < sent->arg_num[t]; a++) { 
                sent->args[t][a] = ps->nodes[t].args[a];
                sent->arg_roles[t][a] = ps->nodes[t].arg_roles[a];
                strcpy(sent->s_arg_roles[t][a], mp->s_arg_role[bank][sent->arg_roles[t][a]]);
            }
        } else {
            strcpy(sent->s_sense[t], "_");
        }
    }
}
int ps_get_degree(PART_SRL *ps, int w) {
    return  ps->nodes[w].head_num + ps->nodes[w].arg_num;
}


