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

int zcrypt(int argc, char *argv[]) {
  char *fname = NULL;
  int error = FALSE;
  int zephyr = FALSE;
  char *class = NULL, *instance = NULL;
  int mode = M_NONE;

  extern int optind, opterr;
  extern char *optarg;
  char c;

  int messageflag = FALSE;
  ZWRITEOPTIONS zoptions;
  zoptions.flags = 0;

  while ((c = getopt(argc, argv, "ZDERSF:c:i:advqtluons:f:m")) != (char)EOF) {
    switch(c) {
      case 'Z':
        /* Zephyr encrypt */
        mode = M_ZEPHYR_ENCRYPT;
	break;
      case 'D':
        /* Decrypt */
	mode = M_DECRYPT;
	break;
      case 'E':
	/* Encrypt */
	mode = M_ENCRYPT;
	break;
      case 'R':
	/* Randomize the keyfile */
	mode = M_RANDOMIZE;
	break;
      case 'S':
	/* Set a new key value from stdin */
	mode = M_SETKEY;
	break;
      case 'F':
	/* Specify the keyfile explicitly */
	if (fname != NULL) error = TRUE;
	fname = optarg;
	break;
      case 'c':
	/* Zwrite/zcrypt: class name */
	if (class != NULL) error = TRUE;
	class = optarg;
	break;
      case 'i':
	/* Zwrite/zcrypt: instance name */
	if (instance != NULL) error = TRUE;
	instance = optarg;
	break;
      case 'a':
	/* Zwrite: authenticate (default) */
	zoptions.flags &= ~ZWRITE_OPT_NOAUTH;
	break;
      case 'd':
	/* Zwrite: do not authenticate */
	zoptions.flags |= ZWRITE_OPT_NOAUTH;
	break;
      case 'v':
	/* Zwrite: verbose */
	zoptions.flags |= ZWRITE_OPT_VERBOSE;
	break;
      case 'q':
	/* Zwrite: quiet */
	zoptions.flags |= ZWRITE_OPT_QUIET;
	break;
      case 't':
	/* Zwrite: no expand tabs (ignored) */
	break;
      case 'l':
	/* Zwrite: ignore '.' on a line by itself (ignored) */
	zoptions.flags |= ZCRYPT_OPT_IGNOREDOT;
	break;
      case 'u':
	/* Zwrite: urgent message */
	instance = "URGENT";
	break;
      case 'o':
	/* Zwrite: ignore zephyr variables zwrite-class, zwrite-inst, */
	/*         zwrite-opcode */
	zoptions.flags |= ZWRITE_OPT_IGNOREVARS;
	break;
      case 'n':
	/* Zwrite: prevent PING message (always used) */
	break;
      case 's':
	/* Zwrite: signature */
	zoptions.flags |= ZWRITE_OPT_SIGNATURE;
	zoptions.signature = optarg;
	break;
      case 'f':
	/* Zwrite: file system specification (ignored) */
	break;
      case 'm':
	/* Message on rest of line */
	messageflag = TRUE;
	break;
      case '?':
	error = TRUE;
	break;
    }
    if (error || messageflag) break;
  }

  if (class != NULL || instance != NULL) {
    zephyr = TRUE;
  }

  if (messageflag) {
    zoptions.flags |= ZCRYPT_OPT_MESSAGE;
    zoptions.message = BuildArgString(argv, optind, argc);
    if (!zoptions.message)
    {
      printf("Memory allocation error.\n");
      error = TRUE;
    }
  } else if (optind < argc) {
    error = TRUE;
  }

  if (mode == M_NONE) mode = (zephyr?M_ZEPHYR_ENCRYPT:M_ENCRYPT);

  if (mode == M_ZEPHYR_ENCRYPT && !zephyr) error = TRUE;

  if (!error && fname == NULL && (class != NULL || instance != NULL)) {
    fname = GetZephyrVarKeyFile(argv[0], class, instance);
  }

  
  if (error || fname == NULL) {
    printf("Usage: %s [-Z|-D|-E|-R|-S] [-F Keyfile] [-c class] [-i instance]\n", argv[0]);
    printf("       [-advqtluon] [-s signature] [-f arg] [-m message]\n");
    printf("  One or more of class, instance, and keyfile must be specified.\n");
  } else {
    if (mode == M_RANDOMIZE) {
      /* Choose a new, random key */
/*
      FILE *fkey = fopen(fname, "w");
      if (!fkey)
	printf("Could not open key file for writing: %s\n", fname);
      else
      {
	char string[100];
	fputs(fkey, string);
	fclose(fkey);
	}
 */
      printf("Feature not yet implemented.\n");
    } else if (mode == M_SETKEY) {
      /* Set a new, user-entered key */
      char newkey[MAX_KEY];
      FILE *fkey;
      
      if (isatty(0)) {
	printf("Enter new key: ");
	/* Really should read without echo!!! */
	fgets(newkey, MAX_KEY - 1, stdin);
      } else {
	fgets(newkey, MAX_KEY - 1, stdin);
      }

      fkey = fopen(fname, "w");
      if (!fkey) {
	printf("Could not open key file for writing: %s\n", fname);
      } else {
	if (fputs(newkey, fkey) != strlen(newkey) || putc('\n', fkey) != '\n') {
	  printf("Error writing to key file.\n");
	  fclose(fkey);
	} else {
	  fclose(fkey);
	  printf("Key update complete.\n");
	}
      }
    } else {
      /* Encrypt/decrypt */
      FILE *fkey = fopen(fname, "r");
      if (!fkey) {
	printf("Could not open key file: %s\n", fname);
      } else {
	char keystring[MAX_KEY];
	fgets(keystring, MAX_KEY-1, fkey);
	if (mode == M_ZEPHYR_ENCRYPT || mode == M_ENCRYPT) {
	  do_encrypt(keystring, (mode == M_ZEPHYR_ENCRYPT), class, instance,
		     &zoptions, fname);
	} else {
	  do_decrypt(keystring);
	}
	fclose(fkey);
      }
    }
  }

  /* Always print the **END** message if -D is specified. */
  if (mode == M_DECRYPT) printf("**END**\n");

  exit(0);
}

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

/* Build a space-separated string from argv from elements between start  *
 * and end - 1.  owl_malloc()'s the returned string. */
char *BuildArgString(char **argv, int start, int end) {
  int len = 1;
  int i;
  char *result;

  /* Compute the length of the string.  (Plus 1 or 2) */
  for (i = start; i < end; i++) {
    len += strlen(argv[i]) + 1;
  }

  /* Allocate memory */
  result = (char *)owl_malloc(len);
  if (result) {
    /* Build the string */
    char *ptr = result;
    /* Start with an empty string, in case nothing is copied. */
    *ptr = '\0';
    /* Copy the arguments */
    for (i = start; i < end; i++) {
      char *temp = argv[i];
      /* Add a space, if not the first argument */
      if (i != start) *ptr++ = ' ';
      /* Copy argv[i], leaving ptr pointing to the '\0' copied from temp */
      while ((*ptr = *temp++)!=0) {
	ptr++;
      }
    }
  }

  return(result);
}

#define MAX_BUFF 258
#define MAX_SEARCH 3
/* Find the class/instance in the .crypt-table */
char *GetZephyrVarKeyFile(char *whoami, char *class, char *instance) {
  char *keyfile = NULL;
  char varname[MAX_SEARCH][128];
  int length[MAX_SEARCH], i;
  char buffer[MAX_BUFF];
  char filename[MAX_BUFF];
  char result[MAX_SEARCH][MAX_BUFF];
  int numsearch = 0;
  FILE *fsearch;

  /* Determine names to look for in .crypt-table */
  if (instance) {
    sprintf(varname[numsearch++], "crypt-%s-%s:", (class?class:"message"), instance);
  }
  if (class) {
    sprintf(varname[numsearch++], "crypt-%s:", class);
  }
  sprintf(varname[numsearch++], "crypt-default:");

  /* Setup the result array, and determine string lengths */
  for (i = 0; i < numsearch; i++) {
    result[i][0] = '\0';
    length[i] = strlen(varname[i]);
  }

  /* Open~/.crypt-table */
  sprintf(filename, "%s/.crypt-table", getenv("HOME"));
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

  return(keyfile);
}

static pid_t zephyrpipe_pid = 0;

/* Open a pipe to zwrite */
static FILE *GetZephyrPipe(char *class, char *instance, ZWRITEOPTIONS *zoptions) {
  int fildes[2];
  pid_t pid;
  FILE *result;
  char *argv[20];
  int argc = 0;

  if (pipe(fildes) < 0) return(NULL);
  pid = fork();

  if (pid < 0) {
    /* Error: clean up */
    close(fildes[0]);
    close(fildes[1]);
    result = NULL;
  } else if (pid == 0) {
    /* Setup child process */
    argv[argc++] = "zwrite";
    argv[argc++] = "-n";     /* Always send without ping */
    if (class) {
      argv[argc++] = "-c";
      argv[argc++] = class;
    }
    if (instance) {
      argv[argc++] = "-i";
      argv[argc++] = instance;
    }
    if (zoptions->flags & ZWRITE_OPT_NOAUTH) argv[argc++] = "-d";
    if (zoptions->flags & ZWRITE_OPT_QUIET) argv[argc++] = "-q";
    if (zoptions->flags & ZWRITE_OPT_VERBOSE) argv[argc++] = "-v";
    if (zoptions->flags & ZWRITE_OPT_SIGNATURE) {
      argv[argc++] = "-s";
      argv[argc++] = zoptions->signature;
    }
    argv[argc++] = "-O";
    argv[argc++] = "crypt";
    argv[argc] = NULL;
    close(fildes[1]);
    if (fildes[0] != STDIN_FILENO) {
      if (dup2(fildes[0], STDIN_FILENO) != STDIN_FILENO) exit(0);
      close(fildes[0]);
    }
    close(fildes[0]);
    execvp(argv[0], argv);
    printf("Exec error: could not run zwrite\n");
    exit(0);
  } else {
    close(fildes[0]);
    /* Create a FILE * for the zwrite pipe */
    result = (FILE *)fdopen(fildes[1], "w");
    zephyrpipe_pid = pid;
  }

  return(result);
}

/* Close the pipe to zwrite */
void CloseZephyrPipe(FILE *pipe) {
  fclose(pipe);
  waitpid(zephyrpipe_pid, NULL, 0);
  zephyrpipe_pid = 0;
}

#define MAX_RESULT 2048


void block_to_ascii(char *output, FILE *outfile) {
  int i;
  for (i = 0; i < 8; i++) {
    putc(((output[i] & 0xf0) >> 4) + BASE_CODE, outfile);
    putc( (output[i] & 0x0f)       + BASE_CODE, outfile);
  }
}

#define MAX_LINE 128

/* Encrypt stdin, with prompt if isatty, and send to stdout, or to zwrite
   if zephyr is set. */
static int do_encrypt(char *keystring, int zephyr, char *class, char *instance, ZWRITEOPTIONS *zoptions, char* keyfile) {
  des_cblock key;
  des_key_schedule schedule;
  char input[8], output[8];
  int size;
  FILE *outfile = stdout;
  int error = FALSE;
  char *inbuff = NULL, *inptr;
  int freein = FALSE;
  int use_buffer = FALSE;
  int num_blocks=0, last_block_size=0;

  des_string_to_key(keystring, key);
  des_key_sched(key, schedule);

  if (zephyr) {
    if (zoptions->flags & ZCRYPT_OPT_MESSAGE) {
      /* Use the -m message */
      int length;
      inbuff = zoptions->message;      
      length = strlen(inbuff);
      num_blocks = (length + 7) / 8;
      last_block_size = ((length + 7) % 8) + 1;
      use_buffer = TRUE;
    } else if (isatty(0)) {
      /* tty input, so show the "Type your message now..." message */
      if (zoptions->flags & ZCRYPT_OPT_IGNOREDOT) {
	printf("Type your message now.  End with the end-of-file character.\n");
      } else {
	printf("Type your message now.  End with control-D or a dot on a line by itself.\n");
      }
      use_buffer = TRUE;
      if ((inptr = inbuff = (char *)owl_malloc(MAX_RESULT)) == NULL) {
	printf("Memory allocation error\n");
	return FALSE;
      }
      while (inptr - inbuff < MAX_RESULT - MAX_LINE - 20) {
	fgets(inptr, MAX_LINE, stdin);
	if (inptr[0]) {
	  if (inptr[0] == '.' && inptr[1] == '\n' && 
	      !(zoptions->flags & ZCRYPT_OPT_IGNOREDOT)) {
	    inptr[0] = '\0';
	    break;
	  } else {
	    inptr += strlen(inptr);
	  }
	} else {
	  break;
	}
      }
      num_blocks = (inptr - inbuff + 7) / 8;
      last_block_size = ((inptr - inbuff + 7) % 8) + 1;
      freein = TRUE;
    }

    /* if (zephyr) */
    outfile = GetZephyrPipe(class, instance, zoptions);
    if (!outfile) {
      printf("Could not run zwrite\n");
      if (freein && inbuff) {
	owl_free(inbuff);
      }
      return(FALSE);
    }
  }

  inptr = inbuff;

  /* Encrypt the input (inbuff or stdin) and send it to outfile */
  while (TRUE) {
    if (use_buffer) {
      /* Get 8 bytes from buffer */
      if (num_blocks > 1) {
	size = 8;
	memcpy(input, inptr, size);
	inptr += 8;
	num_blocks--;
      } else if (num_blocks == 1) {
	size = last_block_size;
	memcpy(input, inptr, size);
	num_blocks--;
      } else {
	size = 0;
      }
    } else {
      /* Get 8 bytes from stdin */
      size = fread(input, 1, 8, stdin);
    }

    /* Check for EOF and pad the string to 8 chars, if needed */
    if (size == 0) break;     /* END OF INPUT: BREAK FROM while LOOP! */
      
    if (size < 8) memset(input + size, 0, 8 - size);

    /* Encrypt and output the block */
    des_ecb_encrypt(input, output, schedule, TRUE);
    block_to_ascii(output, outfile);

    if (size < 8) break;
  }

  /* Close out the output */
  if (!error) putc('\n', outfile);
  if (zephyr) CloseZephyrPipe(outfile);

  /* Free the input buffer, if necessary */
  if (freein && inbuff) owl_free(inbuff);

  return(!error);
}

/* Read a half-byte from stdin, skipping invalid characters.  Returns -1
   if at EOF or file error */
int read_ascii_nybble() {
  char c;

  while (TRUE) {
    if (fread(&c, 1, 1, stdin) == 0) {
      return(-1);
    } else if (c >= BASE_CODE && c <= LAST_CODE) {
      return(c - BASE_CODE);
    }
  }
}

/* Read both halves of the byte and return the single byte.  Returns -1
   if at EOF or file error. */
int read_ascii_byte() {
  int c1, c2;
  c1 = read_ascii_nybble();
  if (c1 >= 0) {
    c2 = read_ascii_nybble();
    if (c2 >= 0) {
      return c1 * 0x10 + c2;
    }
  }
  return(-1);
}

/* Read an 8-byte DES block from stdin */
int read_ascii_block(char *input) {
  int c, i;

  for (i = 0; i < 8; i++) {
    c = read_ascii_byte();
    if (c < 0) return(FALSE);
    input[i] = c;
  }

  return(TRUE);
}

/* Decrypt stdin */
int do_decrypt(char *keystring) {
  des_cblock key;
  des_key_schedule schedule;
  char input[8], output[9];

  output[0] = '\0';    /* In case no message at all                 */
  output[8] = '\0';    /* NULL at end will limit string length to 8 */

  des_string_to_key(keystring, key);
  des_key_sched(key, schedule);

  while (read_ascii_block(input)) {
    des_ecb_encrypt(input, output, schedule, FALSE);
    printf("%s", output);
  }

  if (output[0]) {
    if (output[strlen(output)-1] != '\n') {
      printf("\n");
    }
  } else {
    printf("\n");
  }
  return(0);
}

#endif
