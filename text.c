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


/* caller must free the return */
char *owl_text_htmlstrip(char *in) {
  char *ptr1, *end, *ptr2, *ptr3, *out;

  out=owl_malloc(strlen(in)+30);
  strcpy(out, "");

  ptr1=in;
  end=in+strlen(in);
  
  while(ptr1<end) {
    /* look for an open bracket */
    ptr2=strchr(ptr1, '<');

    /* if none, copy in from here to end and exit */
    if (ptr2==NULL) {
      strcat(out, ptr1);
      return(out);
    }

    /* otherwise copy in everything before the open bracket */
    if (ptr2>ptr1) {
      strncat(out, ptr1, ptr2-ptr1);
    }

    /* find the close bracket */
    ptr3=strchr(ptr2, '>');
    
    /* if there is no close, copy as you are and exit */
    if (!ptr3) {
      strcat(out, ptr2);
      return(out);
    }

    /* look for things we know */
    if (!strncasecmp(ptr2, "<BODY ", 6) ||
	!strncasecmp(ptr2, "<FONT", 5) ||
	!strncasecmp(ptr2, "<HTML", 5) ||
	!strncasecmp(ptr2, "</FONT", 6) ||
	!strncasecmp(ptr2, "</HTML", 6) ||
	!strncasecmp(ptr2, "</BODY", 6)) {

      /* advance to beyond the angle brakcet and go again */
      ptr1=ptr3+1;
      continue;
    }
    if (!strncasecmp(ptr2, "<BR>", 4)) {
      strcat(out, "\n");
      ptr1=ptr3+1;
      continue;
    }

    /* if it wasn't something we know, copy to the > and  go again */
    strncat(out, ptr2, ptr3-ptr2+1);
    ptr1=ptr3+1;
  }
  return(out);
}
