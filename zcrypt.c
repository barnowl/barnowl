/* This file is stolen and slightly modified code */

/* zcrypt.c -- Read in a data stream from stdin & dump a decrypted/encrypted *
 *   datastream.  Reads the string to make the key from from the first       *
 *   parameter.  Encrypts or decrypts according to -d or -e flag.  (-e is    *
 *   default.)  Will invoke zwrite if the -c option is provided for          *
 *   encryption.  If a zephyr class is specified & the keyfile name omitted  *
 *   the ~/.crypt-table will be checked for "crypt-classname" and then       *
 *   "crypt-default" for the keyfile name.                                   */

static const char fileIdent[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "owl.h"

#ifdef OWL_ENABLE_ZCRYPT

#define BASE_CODE 70
#define LAST_CODE (BASE_CODE + 15)
#define OUTPUT_BLOCK_SIZE 16
#include <unistd.h>
#include <sys/types.h>
#include <des.h>

#define MAX_KEY 128

#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define ZWRITE_OPT_NOAUTH     (1<<0)
#define ZWRITE_OPT_SIGNATURE  (1<<1)
#define ZWRITE_OPT_IGNOREVARS (1<<2)
#define ZWRITE_OPT_VERBOSE    (1<<3)
#define ZWRITE_OPT_QUIET      (1<<4)
#define ZCRYPT_OPT_MESSAGE    (1<<5)
#define ZCRYPT_OPT_IGNOREDOT  (1<<6)

typedef struct
{
  int flags;
  char *signature;
  char *message;
} ZWRITEOPTIONS;

char *GetZephyrVarKeyFile(char *whoami, char *class, char *instance);
char *BuildArgString(char **argv, int start, int end);
static int do_encrypt(char *keystring, int zephyr, char *class, char *instance, ZWRITEOPTIONS *zoptions, char* keyfile);
int do_decrypt(char *keystring);

#ifndef HAVE_DES_ECB_ENCRYPT_PROTO
int des_ecb_encrypt(char [], char [], des_key_schedule, int);
#endif

#define M_NONE            0
#define M_ZEPHYR_ENCRYPT  1
#define M_DECRYPT         2
#define M_ENCRYPT         3
#define M_RANDOMIZE       4
#define M_SETKEY          5

/* The 'owl_zcrypt_decrypt' function was written by kretch for Owl.
 * Decrypt the message in 'in' on class 'class' and instance
 * 'instance' and leave the result in 'out'.  Out must be a buffer
 * allocated by the caller.
 *
 * return 0 on success, otherwise -1
 */
int owl_zcrypt_decrypt(char *out, char *in, char *class, char *instance) {
  char *fname, keystring[MAX_KEY], *inptr, *endptr;
  FILE *fkey;
  des_cblock key;
  des_key_schedule schedule;
  char input[8], output[9];
  int i, c1, c2;
  
  fname=GetZephyrVarKeyFile("zcrypt", class, instance);
  if (!fname) return(-1);
  fkey=fopen(fname, "r");
  if (!fkey) return(-1);
  fgets(keystring, MAX_KEY-1, fkey);
  fclose(fkey);

  strcpy(out, "");

  output[0] = '\0';    /* In case no message at all                 */
  output[8] = '\0';    /* NULL at end will limit string length to 8 */

  des_string_to_key(keystring, key);
  des_key_sched(key, schedule);

  inptr=in;
  endptr=in+strlen(in)-1;
  while (inptr<endptr) {
    for (i=0; i<8; i++) {
      c1=(inptr[0])-BASE_CODE;
      c2=(inptr[1])-BASE_CODE;
      input[i]=c1 * 0x10 + c2;
      inptr+=2;
    }
    des_ecb_encrypt(input, output, schedule, FALSE);
    strcat(out, output);
  }

  if (output[0]) {
    if (output[strlen(output)-1] != '\n') {
      strcat(out, "\n");
    }
  } else {
    strcat(out, "\n");
  }
  return(0);
}

int owl_zcrypt_encrypt(char *out, char *in, char *class, char *instance) {
  /*  static int do_encrypt(char *keystring, int zephyr, char *class, char *instance, ZWRITEOPTIONS *zoptions, char* keyfile) { */
  char *fname, keystring[MAX_KEY];
  FILE *fkey;
  des_cblock key;
  des_key_schedule schedule;
  char input[8], output[8];
  int size, length, i;
  char *inbuff = NULL, *inptr;
  int use_buffer = FALSE;
  int num_blocks=0, last_block_size=0;

  fname=GetZephyrVarKeyFile("zcrypt", class, instance);
  if (!fname) return(-1);
  fkey=fopen(fname, "r");
  if (!fkey) return(-1);
  fgets(keystring, MAX_KEY-1, fkey);
  fclose(fkey);

  des_string_to_key(keystring, key);
  des_key_sched(key, schedule);

  inbuff=in;
  length=strlen(inbuff);
  num_blocks=(length+7)/8;
  last_block_size=((length+7)%8)+1;
  use_buffer=TRUE;

  strcpy(out, "");
  
  inptr=inbuff;
  while (TRUE) {
    /* Get 8 bytes from buffer */
    if (num_blocks > 1) {
      size = 8;
      memcpy(input, inptr, size);
      inptr+=8;
      num_blocks--;
    } else if (num_blocks == 1) {
      size=last_block_size;
      memcpy(input, inptr, size);
      num_blocks--;
    } else {
      size=0;
    }

    /* Check for EOF and pad the string to 8 chars, if needed */
    if (size == 0) break;     /* END OF INPUT: BREAK FROM while LOOP! */
      
    if (size<8) memset(input + size, 0, 8 - size);

    /* Encrypt and output the block */
    des_ecb_encrypt(input, output, schedule, TRUE);

    for (i = 0; i < 8; i++) {
      sprintf(out + strlen(out), "%c", ((output[i] & 0xf0) >> 4) + BASE_CODE);
      sprintf(out + strlen(out), "%c", (output[i] & 0x0f)        + BASE_CODE);
    }

    if (size < 8) break;
  }
  return(0);
}


#define MAX_BUFF 258
#define MAX_SEARCH 3
/* Find the class/instance in the .crypt-table */
char *GetZephyrVarKeyFile(char *whoami, char *class, char *instance) {
  char *keyfile = NULL;
  char *varname[MAX_SEARCH];
  int length[MAX_SEARCH], i;
  char buffer[MAX_BUFF];
  char *filename;
  char result[MAX_SEARCH][MAX_BUFF];
  int numsearch = 0;
  FILE *fsearch;

  memset(varname, 0, sizeof(varname));

  /* Determine names to look for in .crypt-table */
  if (instance) {
    varname[numsearch++] = owl_sprintf("crypt-%s-%s:", (class?class:"message"), instance);
  }
  if (class) {
    varname[numsearch++] = owl_sprintf("crypt-%s:", class);
  }
  varname[numsearch++] = owl_strdup("crypt-default:");

  /* Setup the result array, and determine string lengths */
  for (i = 0; i < numsearch; i++) {
    result[i][0] = '\0';
    length[i] = strlen(varname[i]);
  }

  /* Open~/.crypt-table */
  filename = owl_sprintf("%s/.crypt-table", getenv("HOME"));
  fsearch = fopen(filename, "r");
  if (fsearch) {
    /* Scan file for a match */
    while (!feof(fsearch)) {
      fgets(buffer, MAX_BUFF - 3, fsearch);
      for (i = 0; i < numsearch; i++) {
	if (strncasecmp(varname[i], buffer, length[i]) == 0) {
	  int j;
	  for (j = length[i]; buffer[j] == ' '; j++)
	    ;
	  strcpy(result[i], &buffer[j]);
	  if (*result[i]) {
	    if (result[i][strlen(result[i])-1] == '\n') {
	      result[i][strlen(result[i])-1] = '\0';
	    }
	  }
	}
      }
    }

    /* Pick the "best" match found */
    keyfile = NULL;
    for (i = 0; i < numsearch; i++) {
      if (*result[i]) {
	keyfile = result[i];
	break;
      }
    }

    if (keyfile == NULL) {
      /* printf("Could not find key table entry.\n"); */
    } else {
      /* Prepare result to be returned */
      char *temp = keyfile;
      keyfile = (char *)owl_malloc(strlen(temp) + 1);
      if (keyfile) {
	strcpy(keyfile, temp);
      } else {
	/* printf("Memory allocation error.\n"); */
      }
    }
    
    fclose(fsearch);
  } else {
    /* printf("Could not open key table file: %s\n", filename); */
  }

  for(i = 0; i < MAX_SEARCH; i++) {
    owl_free(varname[i]);
  }

  owl_free(filename);

  return(keyfile);
}

static pid_t zephyrpipe_pid = 0;

#endif
