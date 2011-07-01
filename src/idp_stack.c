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

//---------------   OPERATIONS WITH STACK

// returns ith elem from stack if available, 0 otherwise (i = 0 - is TOP)
int st_look(STACK *st, int i) {
    if (st->size > i) {
        return st->elems[st->size - i - 1];
    } else {
        return 0;
    }
}

// returns top of stack or 0 if not available
int st_peek(STACK *st) {
    return st_look(st, 0);
}

int st_pop(STACK *st) {
    if (st->size > 0) {
        return st->elems[(st->size--) - 1];
    } else {
        return 0;
    }
}


void st_push(STACK *st, int x) {
    ASSERT(st->size < MAX_SENT_LEN);
    st->elems[(st->size++)] = x;
}
