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

//----------------------------  OPERATIONS WITH PART_TREE

PART_TREE *pt_alloc(int sent_len) {
    DEF_ALLOC(pt, PART_TREE);
    pt->len = sent_len;
    ALLOC_ARRAY(pt->nodes, NODE, sent_len + 1);
    int i;
    for (i = 0; i < sent_len + 1; i++) {
        pt->nodes[i].head = 0;
        pt->nodes[i].deprel = ROOT_DEPREL;
        pt->nodes[i].child_num = 0;
        //ALLOC_ARRAY(pt->nodes[i].childs, int, sent_len);
    }
    return pt;
}


PART_TREE *pt_clone(PART_TREE *pt) {
    PART_TREE *ptc = pt_alloc(pt->len);
    memcpy(ptc->right_connections_num, pt->right_connections_num, sizeof(int) * (pt->len + 1));
    memcpy(ptc->nodes, pt->nodes, sizeof(NODE) * (pt->len + 1)); 
    return ptc;
}

void pt_free(PART_TREE **pt) {
/*    int i;
    for (i = 0; i < pt->len + 1; i++) {
        free(pt->nodes[i].childs);
    }     */
    free((*pt)->nodes);
    free(*pt);
    *pt = NULL;
}

//leftmost child of i, zero if no
int pt_lc(PART_TREE *pt, int i) {
    ASSERT(i <= pt->len);
    if (i == 0) {
        return 0;
    }
    if (pt->nodes[i].child_num == 0) {
        return 0;
    } else {
        return pt->nodes[i].childs[0];
    }
}

//leftmost left child of i, zero if no left children
int pt_llc(PART_TREE *pt, int i) {
    ASSERT(i <= pt->len);
    if (i == 0) {
        return 0;
    }
    if (pt->nodes[i].child_num == 0) {
        return 0;
    } else {
        if (pt->nodes[i].childs[0] < i) {
            return pt->nodes[i].childs[0];
        } else {
            return 0;
        }
    }
}


//rightmost child of i, zero if no
int pt_rc(PART_TREE *pt, int i) {
    ASSERT(i <= pt->len);
    if (i == 0) {
        return 0;
    }
    if (pt->nodes[i].child_num == 0) {
        return 0;
    } else {
        return pt->nodes[i].childs[pt->nodes[i].child_num - 1];
    }
}

//rightmost right child of i, zero if no
int pt_rrc(PART_TREE *pt, int i) {
    ASSERT(i <= pt->len);
    if (i == 0) {
        return 0;
    }
    if (pt->nodes[i].child_num == 0) {
        return 0;
    } else {
        if (pt->nodes[i].childs[pt->nodes[i].child_num - 1] > i) {
            return pt->nodes[i].childs[pt->nodes[i].child_num - 1];
        } else {
            return 0;
        }
    }
}

//left sibling of i, zero if no
int pt_ls(PART_TREE *pt, int i) {
    if (i == 0) {
        return 0;
    }
    int h = pt_head(pt, i);
    if (h == 0) {
        return 0;
    }
    
    int c = 0;
    while (pt->nodes[h].childs[c] != i) {
        c++;
    }
    
    if (c == 0) {
        return 0;
    } else {
        return pt->nodes[h].childs[c-1];
    }
}

//right sibling of i, zero if no
int pt_rs(PART_TREE *pt, int i) {
    if (i == 0) {
        return 0;
    }
    int h = pt_head(pt, i);
    if (h == 0) {
        return 0;
    }
    
    int c = 0;
    while (pt->nodes[h].childs[c] != i) {
        c++;
    }
    if (c == pt->nodes[h].child_num - 1) {
        return 0;
    } else {
        return pt->nodes[h].childs[c + 1];
    }
}

void pt_add_link(PART_TREE *pt, int h, int d, int deprel) {
    ASSERT(d > 0 && h >= 0);
    pt->nodes[d].head = h;
    pt->nodes[d].deprel = deprel;

    if (h > 0) {
        int i;
        for (i = pt->nodes[h].child_num - 1; i >= 0; i--) {
            int c_d = pt->nodes[h].childs[i];
            if (c_d > d) {
                pt->nodes[h].childs[i + 1] = c_d;
            } else {
                ASSERT(c_d != d);
                break;     
            }
        }
        
        pt->nodes[h].childs[i + 1] = d;
        pt->nodes[h].child_num++;
    } 
}

void pt_fill_sentence(PART_TREE *pt, MODEL_PARAMS *mp, SENTENCE *sent) {
    ASSERT(pt->len == sent->len);
    int t;
    for (t = 1; t <= sent->len; t++) {
        sent->head[t] = pt->nodes[t].head;
        sent->deprel[t] = pt->nodes[t].deprel;
        if (sent->deprel[t] == ROOT_DEPREL) {
            strcpy(sent->s_deprel[t], mp->s_deprel_root);
        } else {
            strcpy(sent->s_deprel[t], mp->s_deprel[sent->deprel[t]]);
        }
    }
}


//returns undericted path lentght between to nodes or -0 if no existrs
// path is marked by _prev_node labels in NODES (if > 0), search back from j
// (breadth-first  traversal, assumes that PT is a tree (no undirected loops)
int pt_get_nondirected_path(PART_TREE  *pt, int i, int j, int max_distance) {
    DEF_ALLOC(q, QUEUE);
    q_init(q);
    q_push(q, i);
    pt->nodes[i]._distance = 0;
    pt->nodes[i]._prev_node = -1;
    int distance = -1;

    while (q_peek(q) != 0) {
        int n = q_pop(q);
        // if it is on max_distance then any not processed 
        // neigbour is further than max_distance
        if (pt->nodes[n]._distance >= max_distance) {
            break;
        }
        // process children
        int c;
        for (c = 0; c < pt->nodes[n].child_num; c++) {
            int c_id = pt->nodes[n].childs[c];
            if (c_id !=  pt->nodes[n]._prev_node) {
                pt->nodes[c_id]._distance = pt->nodes[n]._distance + 1;
                pt->nodes[c_id]._prev_node = n;
                q_push(q, c_id);
                if (c_id == j) {
                    distance = pt->nodes[c_id]._distance;
                    break;
                }
            }             
        }
        
        if (distance >= 0) break;

        //check head
        int n_head = pt->nodes[n].head;
        if (n_head != 0 && n_head !=  pt->nodes[n]._prev_node) {
            pt->nodes[n_head]._distance = pt->nodes[n]._distance + 1;
            pt->nodes[n_head]._prev_node = n;
            q_push(q, n_head);
            if (n_head == j) {
                 distance = pt->nodes[n_head]._distance;
                 break;
            }
        }

    }
    free(q);
    return distance;
}

int pt_get_degree(PART_TREE *pt, int w) {
    int c = pt->nodes[w].child_num;
    if (pt->nodes[w].head > 0) {
        c++;
    }
    return c;
}

int pt_is_head_of(PART_TREE *pt, int h, int a) {
    return pt->nodes[a].head == h;
}
