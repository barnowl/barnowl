#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_text_truncate_lines(char *out, char *in, int aline, int lines) {
  /* start with line aline (where the first line is 1) and print
   *  'lines' lines
   */
  char *ptr1, *ptr2;
  int i;

  strcpy(out, "");
  
  if (aline==0) aline=1; /* really illegal use */

  /* find the starting line */
  ptr1=in;
  if (aline!=1) {
     for (i=0; i<aline-1; i++) {
      ptr1=strchr(ptr1, '\n');
      if (!ptr1) return(-1);
      ptr1++;
    }
  }
  /* ptr1 now holds the starting point */

  /* copy in the next 'lines' lines */
  if (lines<1) return(-1);
  
  for (i=0; i<lines; i++) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      strcat(out, ptr1);
      return(-1);
    }
    strncat(out, ptr1, ptr2-ptr1+1);
    ptr1=ptr2+1;
  }
  return(0);
}

void owl_text_truncate_cols(char *out, char *in, int acol, int bcol) {
  char *ptr1, *ptr2, *tmpbuff, *last;
  int len;
  
  /* the first column is column 0 */

  /* the message is expected to end in a new line for now */

  tmpbuff=owl_malloc(strlen(in)+20);

  strcpy(tmpbuff, "");
  last=in+strlen(in)-1;
  ptr1=in;
  while (ptr1<last) {
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      /* but this shouldn't happen if we end in a \n */
      break;
    }
    
    if (ptr2==ptr1) {
      strcat(tmpbuff, "\n");
      ptr1++;
      continue;
    }

    /* we need to check that we won't run over here */
    if ( (ptr2-ptr1) < (bcol-acol) ) {
      len=ptr2-(ptr1+acol);
    } else {
      len=bcol-acol;
    }
    if ((ptr1+len)>=last) {
      len-=last-(ptr1+len);
    }

    strncat(tmpbuff, ptr1+acol, len);
    strcat(tmpbuff, "\n");

    ptr1=ptr2+1;
  }
  strcpy(out, tmpbuff);
  owl_free(tmpbuff);
}


void owl_text_indent(char *out, char *in, int n) {
  char *ptr1, *ptr2, *last;
  int i;

  strcpy(out, "");

  last=in+strlen(in)-1;
  ptr1=in;
  while (ptr1<=last) {
    for (i=0; i<n; i++) {
      strcat(out, " ");
    }
    ptr2=strchr(ptr1, '\n');
    if (!ptr2) {
      strcat(out, ptr1);
      break;
    } else {
      strncat(out, ptr1, ptr2-ptr1+1);
    }
    ptr1=ptr2+1;
  }
}


int owl_text_num_lines(char *in) {
  int lines, i;

  lines=0;
  for (i=0; in[i]!='\0'; i++) {
    if (in[i]=='\n') lines++;
  }

  /* if the last char wasn't a \n there's one more line */
  if (in[i-1]!='\n') lines++;

  return(lines);
}

