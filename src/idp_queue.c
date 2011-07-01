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

//---------------   OPERATIONS WITH QUEUE
// ------- TAKE INTO ACCOUNT THAT THIS VERY ROUGH QUEUE (NOT MORE THAN SIZE OPERATIONS ARE ALLOWED --
// BECAUSE THE QUEUE IS NOT ALLOWED TO WRAP AROUND THE ARRAY

//init queue
void q_init(QUEUE *q) {
    q->front = q->end = 0;
}

// returns ith elem from the queue if available, 0 otherwise (i = 0 - is FRONT)
int q_look(QUEUE *q, int i) {
    if (q->front + i < q->end) {
        return q->elems[q->front +  i];
    } else {
        return 0;
    }
}

// returns fron  of queue or 0 if not available
int q_peek(QUEUE *q) {
    return q_look(q, 0);
}

int q_pop(QUEUE *q) {
    if (q->front < q->end && q->front < MAX_SENT_LEN) {
        return q->elems[(q->front++)];
    } else {
        return 0;
    }
}


void q_push(QUEUE *q, int x) {
    ASSERT(q->end < MAX_SENT_LEN);
    q->elems[(q->end++)] = x;
}
