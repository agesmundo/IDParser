/* read in lines from stdin and write out something to stdout */

/* email description: 
  First it reads in the links for a sentence, then it optionally prints out
  the structure (see compilation options) and checks for nonprojectivity.  To
  do this check, it goes through the sentence from left to right.  At each
  word it first looks at links for which the word is the right element, and
  tries to pop these links from its stack (in the right order).  Then it looks
  at links for which this word is the left element, and pushes these links on
  its stack (in the reverse order).  When it finds a conflict, it tries to
  blame this nonprojectivity on one or more of the links involved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define NOPREF    1 // mark all crossing links as nonprojective
#define SYNPREF   0 // prefer dependency links for choosing projective links
#define HEADPREF  0 // check if moving one end of one link to its head will
                    // uncross the two current links

#define NODIRPREF 0 // both links are bad, if it comes to choosing by direction
#define LEFTWARD  0 // direction of dis-preference for choosing projective links
#define SYNTAX    1 // include dependency links
#define SEMANTICS 0 // include SRL links
#define CHART     0 // display chart of links
#define GRAPH     0 // display graph of links
#define DISPMODS  0 // display link modifications "^c "
#define DISPCONTEXT 0 // display context infor about nonprojective links "^x "
#define DISPSTUFF 1 // display some interesting things "^s "
#define DEBUG     0 // debugging output


struct link {
  int lft; // index
  int rgt; //index
  int syn; // flag
  int lftarc; // flag
  char *label; // label string
};
struct link *create_link(int lft, int rgt, int syn, int lftarc, char *lab)
{
    struct link *res;
    res = (struct link *) malloc(sizeof(struct link));
    if (res == NULL)
        error("Cannot allocate enough space for max number of links.\n");
    res->lft = lft; 
    res->rgt = rgt;
    res->syn = syn;
    res->lftarc = lftarc;
    res->label = lab;
    return res;
}
/* doesn't test label */
int equal_link(struct link *lnk, int lft, int rgt, int syn, int lftarc)
{
    if (lnk == NULL)
      return 0;
    else if (lnk->lft == lft && 
	     lnk->rgt == rgt &&
	     lnk->syn == syn &&
	     lnk->lftarc == lftarc)
      return 1;
    else 
      return 0;
}

void bugbreak() {}
void usage_error(char *);
void test_links(int , int );
int verify_stack(int , struct link **, int , int , int , int );
int contains_elem(int , int , int , int , int , struct link **);
int remove_elem(int , int , int , int , int , struct link **);
int remove_pos(int , int , struct link **);
char *syn_sym(int );
char *lft_sym(int );
char *gov_sym(int );
void display_chart_header(int );
void display_chart(int , int );
char *link_label(int , int , int , int );

// field positions in input format
#define HEAD 9
#define DEPREL 10
#define DEP 11
#define ROLES 12
// max sizes
#define MAXLEN 1000
#define MAXWORD 200


int head[MAXLEN]; // head[dep], dependencies
char deplabel[MAXLEN][MAXWORD]; // deplabel[dep], dependency label
int pred[MAXLEN]; // pred[prednum], SRL predicates
char predlabel[MAXLEN][MAXWORD]; // predlabel[prednum], SRL predicate name
char role[MAXLEN][MAXLEN][MAXWORD]; // role[prednum][dep], SRL role label or ""
char words[MAXLEN][MAXWORD]; // words[position]
int totlinks; // to check other counts
int totproj[2][2], totnonproj[2][2]; // ...[syn][lftarc], counts
int headnonproj[2][2]; // headnonproj[syn][governer], count

int main(int argc, char *argv[])
{
  /*   FILE *infp; */
  char buffer[1000];
  int i, j;
  int f; // input field
  int ind, oldind; // word index
  int p; // predicate index
  char *str;
  int totp, totn;  // counts
  int sentnum;
  
  if (argc != 1)
    usage_error(argv[0]);

  for (i = 0; i < 1000; i++)
    for (j = 0; j < 1000; j++)
      *role[i][j] = '\0';
  for (i = 0; i <= 1; i++) {
    for (j = 0; j <= 1; j++) {
      totproj[i][j] = 0;
      totnonproj[i][j] = 0;
      headnonproj[i][j] = 0;
    }
  }
  totlinks = 0;
  sentnum = 0;
  p = 0;
  ind = 0;
  oldind = 0; // flag for zeroth sentence
  head[0] = 0;
  strcpy(words[0], "ROOT");
  while (fgets(buffer, 1000, stdin) != NULL)
    {
      f = 1;
      if ((str = strtok(buffer, " \t\n")) == NULL) { 
	printf("\n");
	continue;
      }
      //printf("f=%d %s\n", f, str); fflush(stdout);
      f++;
      oldind = ind;
      ind = atoi(str);             // index
      //printf("ind=%d\n", ind); fflush(stdout);
      if (ind == 1) {                        // sentence break
	if (oldind != 0) {
	  /* test preceding sentence */
#if CHART
	  display_chart(oldind, p);
#elif GRAPH
	  display_chart_header(oldind); 
#endif
	  test_links(oldind, p);
	}
	/* start new sentence */
	sentnum++;
	printf("sentence %d\n", sentnum);
	p = 0;
	for (i = 0; i < oldind+1; i++)
	  for (j = 0; j < oldind+1; j++)
	    *role[i][j] = '\0';
      }
      if ((str = strtok(NULL, " \t\n")) != NULL) {
	strncpy(words[ind], str, 12); // word string prefix
	words[ind][12] = '\0';
      }
      f++;
      for (; f != HEAD; f++)
	if (strtok(NULL, " \t\n") == NULL) {
	  printf("error\n");
	  fflush(stdout);
	}
      if ((str = strtok(NULL, " \t\n")) != NULL) {
	head[ind] = atoi(str);  // syn head
      }
      //printf("f=%d %s\n", f, str); fflush(stdout);
      f++;
      for (; f != DEPREL; f++)
	if (strtok(NULL, " \t\n") == NULL) {
	  printf("error\n");
	  fflush(stdout);
	}
      if ((str = strtok(NULL, " \t\n")) != NULL) {
	strcpy(deplabel[ind], str);  // syn dependency label
      }
      //printf("f=%d %s\n", f, str); fflush(stdout);
      f++;
#if SEMANTICS
      for (; f != DEP; f++)
	if (strtok(NULL, " \t\n") == NULL) {
	  printf("error\n");
	  fflush(stdout);
	}
      if ((str = strtok(NULL, " \t\n")) != NULL) {
	if (strcmp(str, "_") != 0) {
	  pred[p] = ind;         // predicate numbering
	  strcpy(predlabel[p], str); // predicate name
	  p++;
	}
      }
      //printf("f=%d %s\n", f, str); fflush(stdout);
      f++;
      for (; (str = strtok(NULL, " \t\n")) != NULL; f++) {
	if (strcmp(str, "_") != 0) {
	  strcpy(role[f - ROLES][ind], str);  // role labels
	}
	else {
	  *role[f - ROLES][ind] = '\0'; // ""  
	}
	//printf("f=%d %s\n", f, str); fflush(stdout);
      }
#endif
    }
  if (oldind != 0)
    test_links(ind, p);

  /* display stats */
  printf("\n\nsyn lft  (syn=1:dependencies, syn=0:SRLs, lft=1:leftarcs, lft=0:rightarcs)");
  totp = 0;
  totn = 0;
  for (i = 0; i <= 1; i++) {
    for (j = 0; j <= 1; j++) {
      if (0
#if SYNTAX
	  || i == 1
#endif
#if SEMANTICS
	  || i == 0
#endif
	  )
	printf("\n %d   %d  total edges=%d, total projective=%d, total nonprojective=%d\n        percent nonprojective= %f",
	       i, j,
	       totproj[i][j] + totnonproj[i][j], 
	       totproj[i][j], 
	       totnonproj[i][j], 
	       100.0*(double) totnonproj[i][j] 
	       / (double) (totproj[i][j] + totnonproj[i][j]));
      totp += totproj[i][j];
      totn += totnonproj[i][j];
    }
  }
  printf("\n\nMoving to head of governor/dependent uncrossed links which caused first violation");
  printf("\nsyn governer nonprojective percentage-of-nonprojective");
  for (i = 0; i <= 1; i++) {
    for (j = 0; j <= 1; j++) {
      if (0
#if SYNTAX
	  || i == 1
#endif
#if SEMANTICS
	  || i == 0
#endif
	  )
	printf("\n %d   %d          %d            %f",
	       i, j,
	       headnonproj[i][j], 
	       100.0*(double) headnonproj[i][j] / (double) totn);
    }
  }
  printf("\n\ntotal edges=%d=%d, total projective=%d, total nonprojective=%d\npercent nonprojective= %f\n\n",
	 totlinks, totp + totn, totp, totn, 
	 100.0*(double) totn / (double) (totp + totn));
}


/* collect counts for projective and nonprojective links */
void test_links(int length, int preds) {

  int s;                    // stack depth
  struct link *stack[1000]; // stack of links looking for righthand side
  int ind, rel, p, i;
  char *typ, *dir;

  s = 0;
  for (ind = 0; ind <= length; ind++) {
#if SEMANTICS
    // count self-links 
    for (p = 0; p < preds; p++) {
      if ((pred[p] == ind && *role[p][ind] != '\0')) {
	totlinks++;
	totproj[0][0]++;                              // count it as a rightarc
#if GRAPH
	printf("%15s", "");
	for (i = 0; i < ind; i++)
	  printf("   ");
	printf(" %s ", syn_sym(0));
	printf("\n");
#elif ! CHART
	printf("p %d %d %s%s\n", ind, ind, syn_sym(0), "-"); // projective
#endif
      }
    }
#endif    
    // pop left element (rel) of relations to left of ind
    for (rel = ind - 1; rel >= 0; rel--) {
#if SYNTAX
      if (head[ind] == rel) {
	totlinks++;
	s = verify_stack(s, stack, rel, ind, 1, 0);
      }
      if (head[rel] == ind) {
	totlinks++;
	s = verify_stack(s, stack, rel, ind, 1, 1);
      }
#endif
#if SEMANTICS
      for (p = 0; p < preds; p++) {
	if ((pred[p] == rel && *role[p][ind] != '\0')) {
	  totlinks++;
	  s = verify_stack(s, stack, rel, ind, 0, 0);
	}
      }
      for (p = 0; p < preds; p++) {
	if ((pred[p] == ind && *role[p][rel] != '\0')) {
	  totlinks++;
	  s = verify_stack(s, stack, rel, ind, 0, 1);
	}
      }
#endif
    }
    // push left element (ind) of relations to right of ind 
    // (must be opposite order from popping)
    for (rel = length; rel > ind; rel--) {
#if SEMANTICS
      for (p = 0; p < preds; p++) {
	if (pred[p] == rel && *role[p][ind] != '\0') {
#if DEBUG
	  printf("push %3d,%d (%d)\n", ind, rel, s);
#endif
	  stack[s] = create_link(ind, rel, 0, 1, role[p][ind]);
	  s++;
	}
      }
      for (p = 0; p < preds; p++) {
	if (pred[p] == ind && *role[p][rel] != '\0') {
#if DEBUG
	  printf("push %3d,%d (%d)\n", ind, rel, s);
#endif
	  stack[s] = create_link(ind, rel, 0, 0, role[p][rel]);
	  s++;
	}
      }
#endif
#if SYNTAX
      if (head[ind] == rel) {
#if DEBUG
	printf("push %3d,%d (%d)\n", ind, rel, s);
#endif
	stack[s] = create_link(ind, rel, 1, 1, deplabel[ind]);
	s++;
      }
      if (head[rel] == ind) {
#if DEBUG
	printf("push %3d,%d (%d)\n", ind, rel, s);
#endif
	stack[s] = create_link(ind, rel, 1, 0, deplabel[rel]);
	s++;
      }
#endif
    }
  }
  while (s > 0) {
#if ! CHART && ! GRAPH
    printf("n l=%d %s%s\n",                           // nonprojective left
	   stack[s-1]->lft, syn_sym(stack[s-1]->syn), lft_sym(stack[s-1]->lftarc));
#endif
    s = remove_pos(s-1, s, stack);
  }

}

/* try to pop specified link from the stack, and return new stack depth.
   Count as projective if link is on top.  Otherwise, blame the
   nonprojectivity on somethings and remove it. */
int verify_stack(int s, struct link **stack, 
		 int lft, int rgt, int syn, int lftarc) {
  char *typ, *dir;
  int i, j;
  int bad;
  int depth, pos;

#if DEBUG
  printf("pop  %3d,%d? (%d)\n", lft, rgt, s);
#endif

  typ = syn_sym(syn);
  dir = lft_sym(lftarc);
  
  //printf("s=%d\n", s); fflush(stdout);

  if (s == 0) {                  // empty stack
    //display_bad(lft, rgt, syn, lftarc);
    totnonproj[syn][lftarc]++;
#if DISPSTUFF
    if (lftarc) {
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
    }
    else {
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
    }
#endif
#if GRAPH
    printf("%15s", "");
    for (i = 0; i < lft; i++)
      printf("   ");
    if (lftarc)
      printf("  <");
    else
      printf("  %s", typ);
    for (i = lft+1; i < rgt; i++)
      printf("  +");
    if (lftarc)
      printf(" +%s", typ);
    else
      printf(" +>");
    printf("  %s\n", link_label(lft, rgt, syn, lftarc));
#else
    printf("n %d %d %s%s\n", lft, rgt, typ, dir); // nonprojective right
#endif
    return s;
  }
  else if (equal_link(stack[s-1], lft, rgt, syn, lftarc)) { 
                                       // lft at top of stack, projective case
    //display_good(lft, rgt, syn, lftarc);
    totproj[syn][lftarc]++;
#if DISPSTUFF
    if (lftarc) {
      printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
      printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
    }
    else {
      printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
      printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
    }
#endif
#if GRAPH
    printf("%15s", "");
    for (i = 0; i < lft; i++)
      printf("   ");
    if (lftarc)
      printf("  <");
    else
      printf("  %s", typ);
    for (i = lft+1; i < rgt; i++)
      printf("  -");
    if (lftarc)
      printf(" -%s", typ);
    else
      printf(" ->");
    printf("  %s\n", link_label(lft, rgt, syn, lftarc));
#elif ! CHART
    printf("p %d %d %s%s\n", lft, rgt, typ, dir); // projective
#endif
    s = remove_elem(lft, rgt, syn, lftarc, s, stack);
    return s;
  }
  else if (depth = contains_elem(lft, rgt, syn, lftarc, s, stack)) { 
                                                     // lft burried in stack
#if DEBUG
    printf("%d in stack:", lft);
    for (i = s-1; i >= 0; i--)
      printf(" %d", stack[i]->lft);
    printf("\n");
#endif
    pos = s - depth;
    bad = 0;
    /* go through crossing links */
    for (i = s-1; i > pos; i--) {
      //printf("stack[%d]->lft = %d\n", i, stack[i]->lft);
      if (! ( (stack[i]->lft < lft && stack[i]->rgt > lft && 
	       stack[i]->rgt < rgt)
	      || (stack[i]->lft > lft && stack[i]->lft < rgt && 
		  stack[i]->rgt > rgt))) {
	printf("error: links %d,%d;%d,%d and %d,%d;%d,%d treated as crossing",
	       stack[i]->lft, stack[i]->rgt, stack[i]->syn, stack[i]->lftarc,
	       lft, rgt, syn, lftarc);
      }
#if DISPCONTEXT
      if (lftarc) 
	printf("x %s:%d<%s-%d", syn_sym(syn), lft, link_label(lft, rgt, syn, lftarc), rgt);
      else
	printf("x %s:%d-%s>%d", syn_sym(syn), lft, link_label(lft, rgt, syn, lftarc), rgt);
      printf(" %d<%s-%d", lft, deplabel[lft], head[lft]);
      printf(" %d<%s-%d", rgt, deplabel[rgt], head[rgt]);
      printf("\t");
      if (stack[i]->lftarc) 
	printf("x %s:%d<%s-%d", syn_sym(stack[i]->syn), stack[i]->lft, stack[i]->label, stack[i]->rgt);
      else
	printf("x %s:%d-%s>%d", syn_sym(stack[i]->syn), stack[i]->lft, stack[i]->label, stack[i]->rgt);
      printf(" %d<%s-%d", stack[i]->lft, deplabel[stack[i]->lft], head[stack[i]->lft]);
      printf(" %d<%s-%d", stack[i]->rgt, deplabel[stack[i]->rgt], head[stack[i]->rgt]);
      printf("\n");
#endif      
#if NOPREF
#if DEBUG
      printf("0removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
	     stack[i]->lft, stack[i]->rgt, 
	     stack[i]->syn, stack[i]->lftarc,
	     lft, rgt, syn, lftarc);
#endif
      bad = 1;
      s = remove_pos(i, s, stack);
#else
#if SYNPREF
      if (syn && ! stack[i]->syn) {
#if DEBUG
	printf("1removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
	       stack[i]->lft, stack[i]->rgt, 
	       stack[i]->syn, stack[i]->lftarc,
	       lft, rgt, syn, lftarc);
#endif
	if (lft <= head[stack[i]->rgt] 
	    && rgt >= head[stack[i]->rgt]) {
	  headnonproj[stack[i]->syn][stack[i]->lftarc]++;
#if DISPMODS
	  printf("c %d %d<-%d %s%s %s_%s_%s\n",
		 stack[i]->lft, stack[i]->rgt, head[stack[i]->rgt],
		 syn_sym(stack[i]->syn), lft_sym(stack[i]->lftarc),
		 gov_sym(stack[i]->lftarc), deplabel[stack[i]->rgt],
		 stack[i]->label); 
#endif
	}
	else if (lft >= head[stack[i]->lft]
		 || rgt <= head[stack[i]->lft]) {
	  if (stack[i]->lftarc)
	    headnonproj[stack[i]->syn][0]++;
	  else
	    headnonproj[stack[i]->syn][1]++;
#if DISPMODS
	  printf("c %d<-%d %d %s%s %s_%s_%s\n",
		 stack[i]->lft, head[stack[i]->lft], stack[i]->rgt, 
		 syn_sym(stack[i]->syn), lft_sym(stack[i]->lftarc),
		 gov_sym(! stack[i]->lftarc), deplabel[stack[i]->lft],
		 stack[i]->label); 
#endif
	}
	s = remove_pos(i, s, stack);
      }
      else if (! syn && stack[i]->syn) {
	if (! bad) { // only check first violation
#if DEBUG
	  printf("2removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
		 lft, rgt, syn, lftarc,
		 stack[i]->lft, stack[i]->rgt, 
		 stack[i]->syn, stack[i]->lftarc);
#endif
	  if (stack[i]->lft >= head[rgt] 
	      || stack[i]->rgt <= head[rgt]) {
	    headnonproj[syn][lftarc]++;
#if DISPMODS
	    printf("c %d %d<-%d %s%s %s_%s_%s\n",
		   lft, rgt, head[rgt],
		   syn_sym(syn), lft_sym(lftarc),
		   gov_sym(lftarc), deplabel[rgt],
		   link_label(lft, rgt, syn, lftarc)); 
#endif
	  }
	  else if (stack[i]->lft <= head[lft] 
		   && stack[i]->rgt >= head[lft] ) {
	    if (lftarc)
	      headnonproj[syn][0]++;
	    else
	      headnonproj[syn][1]++;
#if DISPMODS
	    printf("c %d<-%d %d %s%s %s_%s_%s\n",
		   lft, head[lft], rgt, 
		   syn_sym(syn), lft_sym(lftarc),
		   gov_sym(! lftarc), deplabel[lft],
		   link_label(lft, rgt, syn, lftarc)); 
#endif
	  }
	}
	bad = 1;
      }
      else
#endif
	{
#if HEADPREF
	  /* head movements not guaranteed to produce projective structure */
	  if (lft <= head[stack[i]->rgt] 
	      && rgt >= head[stack[i]->rgt]) {
#if DEBUG
	    printf("3removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
		   stack[i]->lft, stack[i]->rgt, 
		   stack[i]->syn, stack[i]->lftarc,
		   lft, rgt, syn, lftarc);
#endif
	    headnonproj[stack[i]->syn][stack[i]->lftarc]++;
#if DISPMODS
	    printf("c %d %d<-%d %s%s %s_%s_%s\n",
		   stack[i]->lft, stack[i]->rgt, head[stack[i]->rgt],
		   syn_sym(stack[i]->syn), lft_sym(stack[i]->lftarc),
		   gov_sym(stack[i]->lftarc), deplabel[stack[i]->rgt],
		   stack[i]->label); 
#endif
	    s = remove_pos(i, s, stack);
	  }
	  else if (lft >= head[stack[i]->lft]
		   || rgt <= head[stack[i]->lft]) {
#if DEBUG
	    printf("4removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
		   stack[i]->lft, stack[i]->rgt, 
		   stack[i]->syn, stack[i]->lftarc,
		   lft, rgt, syn, lftarc);
#endif
	    if (stack[i]->lftarc)
	      headnonproj[stack[i]->syn][0]++;
	    else
	      headnonproj[stack[i]->syn][1]++;
#if DISPMODS
	    printf("c %d<-%d %d %s%s %s_%s_%s\n",
		   stack[i]->lft, head[stack[i]->lft], stack[i]->rgt, 
		   syn_sym(stack[i]->syn), lft_sym(stack[i]->lftarc),
		   gov_sym(! stack[i]->lftarc), deplabel[stack[i]->lft],
		   stack[i]->label); 
#endif
	    s = remove_pos(i, s, stack);
	  }
	  else if (stack[i]->lft >= head[rgt] 
		   || stack[i]->rgt <= head[rgt]) {
	    if (! bad) { // only check first violation
#if DEBUG
	      printf("5removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
		     lft, rgt, syn, lftarc,
		     stack[i]->lft, stack[i]->rgt, 
		     stack[i]->syn, stack[i]->lftarc);
#endif
	      headnonproj[syn][lftarc]++;
#if DISPMODS
	      printf("c %d %d<-%d %s%s %s_%s_%s\n",
		     lft, rgt, head[rgt],
		     syn_sym(syn), lft_sym(lftarc),
		     gov_sym(lftarc), deplabel[rgt],
		     link_label(lft, rgt, syn, lftarc)); 
#endif
	    }
	    bad = 1;
	  }
	  else if (stack[i]->lft <= head[lft] 
		   && stack[i]->rgt >= head[lft] ) {
	    if (! bad) { // only check first violation
#if DEBUG
	      printf("6removing %d,%d;%d,%d due to %d,%d;%d,%d\n", 
		     lft, rgt, syn, lftarc,
		     stack[i]->lft, stack[i]->rgt, 
		     stack[i]->syn, stack[i]->lftarc);
#endif
	      if (lftarc)
		headnonproj[syn][0]++;
	      else
		headnonproj[syn][1]++;
#if DISPMODS
	      printf("c %d<-%d %d %s%s %s_%s_%s\n",
		     lft, head[lft], rgt, 
		     syn_sym(syn), lft_sym(lftarc),
		     gov_sym(! lftarc), deplabel[lft],
		     link_label(lft, rgt, syn, lftarc)); 
#endif
	    }
	    bad = 1;
	  }
	  else
#endif
	    {
#if NODIRPREF
	      s = remove_pos(i, s, stack);
	      bad = 1;
#elif LEFTWARD
	      s = remove_pos(i, s, stack);
#else
	      bad = 1;
#endif	
	    }
	}
#endif	
    }
    if (! equal_link(stack[pos], lft, rgt, syn, lftarc)) {
      printf("this shouldn't happen\n");
    }
    if (bad) {
      //display_bad(lft, rgt, syn, lftarc);
      totnonproj[syn][lftarc]++;
#if DISPSTUFF
      if (lftarc) {
	printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(1), 
	       link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
	printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(0), 
	       link_label(lft, rgt, syn, lftarc), deplabel[lft]);
      }
      else {
	printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(1), 
	       link_label(lft, rgt, syn, lftarc), deplabel[lft]);
	printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(0), 
	       link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
      }
#endif
#if GRAPH
      printf("%15s", "");
      for (i = 0; i < lft; i++)
	printf("   ");
      if (lftarc)
	printf("  <");
      else
	printf("  %s", typ);
      for (i = lft+1; i < rgt; i++)
	printf("  +");
      if (lftarc)
	printf(" +%s", typ);
      else
	printf(" +>");
      printf("  %s\n", link_label(lft, rgt, syn, lftarc));
#else
      printf("n %d %d %s%s\n", lft, rgt, typ, dir); // nonprojective right
#endif
    }
    else {
      //display_good(lft, rgt, syn, lftarc);
      totproj[syn][lftarc]++;
#if DISPSTUFF
      if (lftarc) {
	printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(1), 
	       link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
	printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(0), 
	       link_label(lft, rgt, syn, lftarc), deplabel[lft]);
      }
      else {
	printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(1), 
	       link_label(lft, rgt, syn, lftarc), deplabel[lft]);
	printf("sp %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(0), 
	       link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
      }
#endif
#if GRAPH
      printf("%15s", "");
      for (i = 0; i < lft; i++)
	printf("   ");
      if (lftarc)
	printf("  <");
      else
	printf("  %s", typ);
      for (i = lft+1; i < rgt; i++)
	printf("  -");
      if (lftarc)
	printf(" -%s", typ);
      else
	printf(" ->");
      printf("  %s\n", link_label(lft, rgt, syn, lftarc));
#elif ! CHART
      printf("p %d %d %s%s\n", lft, rgt, typ, dir); // projective
#endif
    }
    s = remove_elem(lft, rgt, syn, lftarc, s, stack);
    return s;

  }
  else {                               // lft not in stack
    //display_bad(lft, rgt, syn, lftarc);
    totnonproj[syn][lftarc]++;
#if DISPSTUFF
    if (lftarc) {
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(1), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
    }
    else {
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(1), 
	     link_label(lft, rgt, syn, lftarc), deplabel[lft]);
      printf("sn %s%s%s: %s %s\n", syn_sym(syn), lft_sym(0), gov_sym(0), 
	     link_label(lft, rgt, syn, lftarc), deplabel[rgt]);
    }
#endif
#if GRAPH
    printf("%15s", "");
    for (i = 0; i < lft; i++)
      printf("   ");
    if (lftarc)
      printf("  <");
    else
      printf("  %s", typ);
    for (i = lft+1; i < rgt; i++)
      printf("  +");
    if (lftarc)
      printf(" +%s", typ);
    else
      printf(" +>");
    printf("  %s\n", link_label(lft, rgt, syn, lftarc));
#else
    printf("n %d %d %s%s\n", lft, rgt, typ, dir); // nonprojective right
#endif
    return s;
  }

  return s; // shouldn't be needed
}

/* returns position relative to top of stack (top = 1, subtop = 2, etc), or 0 */
int contains_elem(int lft, int rgt, int syn, int lftarc, 
		  int s, struct link **stack) {
  int i;

  for (i = s-1; i >= 0; i--) {
    if (equal_link(stack[i], lft, rgt, syn, lftarc)) {
      return s - i;
    }
  }
  return 0;
}

int remove_elem(int lft, int rgt, int syn, int lftarc, 
		int s, struct link **stack) {
  int i, j;

  for (i = s-1; i >= 0; i--) {
    if (equal_link(stack[i], lft, rgt, syn, lftarc)) {
      s = remove_pos(i, s, stack);
      return s;
    }
  }
  printf("error: elem not in stack\n");
  return s;
}

int remove_pos(int pos, int s, struct link **stack) {
  int j;

  if (pos < 0 || pos >= s)
    printf("error: position not in stack\n");
   
#if DEBUG
  printf("pop pos %d\n", pos+1);
#endif
  free(stack[pos]);
  for (j = pos+1; j < s; j++) {
    stack[j-1] = stack[j];
  }
  s--;
  return s;
}

char *syn_sym(int syn) {

  if (syn)
    return "d";
  else
    return "s";
}

char *lft_sym(int lftarc) {

  if (lftarc)
    return "l";
  else
    return "r";
}

char *gov_sym(int gov) {

  if (gov)
    return "g";
  else
    return "a";
}

void display_chart_header(int length) {
  int i, j;

  printf(" to      from: "); 
  for (i = 0; i <= length; i++) {
    printf("%3d", i);
  }
  printf("\n");
}

void display_chart(int length, int preds) {
  int i, j;
  int cd, cs;
  int p;

  display_chart_header(length);
  for (i = 0; i <= length; i++) { // to
    printf("%3d%12s", i, words[i]); 
    for (j = 0; j <= length; j++) { // from
      cd = 0;
      cs = 0;
      //#if SYNTAX
      if (head[i] == j) {
	cd++;
      }
      //#endif
#if SEMANTICS
      for (p = 0; p < preds; p++) {
	if (pred[p] == j && *role[p][i] != '\0') {
	  cs++;
	}
      }
#endif
      if (cd == 0 && cs == 0)
	printf("  -");
      else if (cd == 1 && cs == 0)
	printf("  d");
      else if (cd == 0 && cs == 1)
	printf("  s");
      else if (cd == 1 && cs == 1)
	printf(" ds");
      else if (cd == 2 && cs == 0)
	printf(" dd");
      else if (cd == 0 && cs == 2)
	printf(" ss");
      else if (cd == 2 && cs == 1)
	printf("dds");
      else if (cd == 1 && cs == 2)
	printf("dss");
      else if (cd == 3 && cs == 0)
	printf("ddd");
      else if (cd == 0 && cs == 3)
	printf("sss");
      else
	printf("%3d", cd + cs);
    }
    printf("\n");
  }
}


void usage_error(char *name)
{
  printf("Usage:  %s (-h)\n  data to stdin and stats to stdout.\n", 
	 name);  
  exit(0);
}



char *link_label(int lft, int rgt, int syn, int lftarc) {
  int p;

  if (syn) {
    if (lftarc)
      return deplabel[lft];
    else
      return deplabel[rgt];
  }
  else {
    if (lftarc) {
      for (p = 0; p < MAXLEN; p++) {
	if (pred[p] == rgt) {
	  return role[p][lft];
	}
      }
    }
    else {
      for (p = 0; p < MAXLEN; p++) {
	if (pred[p] == lft) {
	  return role[p][rgt];
	}
      }
    }
  }
  return "";
}

