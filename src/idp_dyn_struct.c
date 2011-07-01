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

//*****************************************************
// Operations with dynamic structure of options
//****************************************************


OPTION_LIST *increase_list(OPTION_LIST *prev_node, OPTION *opt) {
    DEF_ALLOC(next_node, OPTION_LIST);
    next_node->prev = prev_node;
    ASSERT((prev_node == NULL && opt == NULL) || (prev_node != NULL && opt != NULL))
    if (prev_node != NULL) {
        prev_node->next = next_node;
        prev_node->elem = opt;
    }
    return next_node;
}

//adds an element to the sorted list (list is sorted in ascending order of the lprob)
OPTION_LIST *increase_list_asc(OPTION_LIST *tail, OPTION *opt) {
    ASSERT(tail != NULL && opt != NULL);
    DEF_ALLOC(new_node, OPTION_LIST);
    new_node->elem = opt;
    
    OPTION_LIST *curr_node = tail;
    while (curr_node->prev != NULL) {
        if (curr_node->prev->elem->lprob <= opt->lprob) {
            OPTION_LIST *prev_node = curr_node->prev;
            new_node->next = prev_node->next;
            new_node->prev = prev_node;
            new_node->next->prev = new_node;
            new_node->prev->next = new_node;
            break;
        }
        curr_node = curr_node->prev;
    }
    //it is the smallest element in the list
    if (curr_node->prev == NULL) {
        curr_node->prev = new_node;
        new_node->next = curr_node;
    }
    return tail;
}

OPTION_LIST *reduce_list(OPTION_LIST *last_node) {
    OPTION_LIST *prev_node = last_node->prev;
    prev_node->elem = NULL;
    free(last_node);
    return prev_node;
}

void free_option_list(OPTION_LIST *last) {
    if (last == NULL) {
        return;
    }
    OPTION_LIST *prev = NULL, *curr = last;
    while(curr != NULL) {
        prev = curr->prev;
        free(curr);
        curr = prev;
    }
}

void free_option(OPTION *opt) {
    ASSERT(opt != NULL);
    free_option_list(opt->next_options);
    if (opt->previous_option != NULL) {
        OPTION_LIST *node = opt->previous_option->next_options->prev;
        int has_deleted = 0;
        while (node != NULL) {
            if (node->elem == opt) {
                node->next->prev = node->prev;
                if (node->prev != NULL) {
                    node->prev->next = node->next;
                }
                free(node);
                has_deleted = 1;
                break;
            }
            node = node->prev;
        }
        ASSERT(has_deleted)
    }
    if (opt->pt != NULL) {
        pt_free(&(opt->pt));
    }
    if (opt->ps != NULL) {
        ps_free(&(opt->ps));
    }
    
    if (opt->outputs.mask != NULL) {
        free(opt->outputs.mask);
        free(opt->outputs.q);
        free(opt->outputs.del);
        free(opt->outputs.parts);
        free(opt->outputs.exp);
        free(opt->outputs.a);
        free(opt->outputs.b);
    }

    free(opt);
}

void free_options(OPTION *last_opt) {
    ASSERT(last_opt != NULL);
    OPTION *opt = last_opt, *prev_opt = NULL;
    while(opt !=  NULL) {
        prev_opt = opt->previous_option;
        free_option(opt);
        
        //remove opt fro prev_opt->next_options
        //if prev_opt->next_options is not empty after removal - break
        if (prev_opt != NULL) {
            //some next options are still left (e.g. a derivation tree is considered and one branch is freed)
            if (prev_opt->next_options->prev != NULL) {
               break; 
            }
        }
        opt = prev_opt;
    }
}

void free_derivation(OPTION_LIST *last) {
    ASSERT(last->prev != NULL);
    ASSERT(last->prev->elem != NULL);
    OPTION *last_opt = last->prev->elem;
    free_options(last_opt);
    free_option_list(last);
}

OPTION_LIST *concat(OPTION_LIST *ol1, OPTION_LIST *ol2)  {
    ASSERT(ol1 != NULL && ol2 != NULL && ol2->elem == NULL);
    OPTION_LIST *node = ol1;

    while (node->prev != NULL) {
        node = node->prev;
    }
    node->prev = ol2->prev;
    if (ol2->prev != NULL) {
        ol2->prev->next = node;
    }
    free(ol2);
    return ol1;
}

/* prune (and free) anything at or previous to last_opt which has probability
   less than threshold.  Return pruned list, or NULL.*/
OPTION_LIST *prune_options(OPTION_LIST *last_opt, double threshold_lprob) {
  OPTION_LIST *res, *ptr, *ptr2;

  if (last_opt == NULL)
    return NULL;

  if (last_opt->elem == NULL || last_opt->elem->lprob >= threshold_lprob) 
    res = last_opt;
  else
    res = NULL;
  for (ptr = last_opt; ptr != NULL; ptr = ptr->prev) {
    if (ptr->elem != NULL && ptr->elem->lprob < threshold_lprob) {
      for (ptr2 = ptr; ptr2 != NULL; ptr2 = ptr2->prev) {
	OPTION *opt = ptr2->elem;
	pt_free(&(opt->pt));
	ps_free(&(opt->ps));
	opt->pt = NULL; opt->ps = NULL;
	if (opt->next_options == NULL || opt->next_options->prev == NULL) {
	  free_options(opt);
	}
      }
      if (ptr->next != NULL)
	ptr->next->prev = NULL;
      free_option_list(ptr);
      break;
    }
  }
  return res;
}


/* JH: this looks useful, if only I could figure out what it does.  Please
   comment. */
int prune_free(OPTION **best_options, OPTION_LIST *tail, int beam) {
    int i;
    for (i = 0; i < beam; i++) {
        OPTION_LIST *node = tail->prev;
        OPTION_LIST *best_node = NULL;
        double max_lprob = - MAXDOUBLE;
        while (node != NULL) {
            //already in the list
            if (node->elem == NULL) {
                node = node->prev;
                continue;
            }
            if (max_lprob < node->elem->lprob) {
                max_lprob = node->elem->lprob;
                best_node = node;
            }
            node = node->prev;
        }
        //empty list
        if (best_node == NULL) {
            break;
        }
        best_options[i] = best_node->elem;
        best_node->elem = NULL;
    }

    OPTION_LIST *node = tail->prev;

    while (node != NULL) {
        if (node->elem != NULL) {
            free_options(node->elem);
        }
        node = node->prev;
    }
    free_option_list(tail);

    return i;
}



