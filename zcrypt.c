/* zcrypt.c -- Read in a data stream from stdin & dump a decrypted/encrypted *
 *   datastream.  Reads the string to make the key from from the first       *
 *   parameter.  Encrypts or decrypts according to -d or -e flag.  (-e is    *
 *   default.)  Will invoke zwrite if the -c option is provided for          *
 *   encryption.  If a zephyr class is specified & the keyfile name omitted  *
 *   the ~/.crypt-table will be checked for "crypt-classname" and then       *
 *   "crypt-default" for the keyfile name.                                   */

#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <zephyr/zephyr.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <ctype.h>

#include "config.h"

#ifdef HAVE_KERBEROS_IV
#include <kerberosIV/des.h>
#else
#include <openssl/des.h>
#endif

#include "filterproc.h"

#define MAX_KEY      128
#define MAX_LINE     128
#define MAX_RESULT   4096

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

char *GetZephyrVarKeyFile(const char *whoami, const char *class, const char *instance);
int ParseCryptSpec(const char *spec, const char **keyfile);
char *BuildArgString(char **argv, int start, int end);
char *read_keystring(const char *keyfile);

int do_encrypt(int zephyr, const char *class, const char *instance,
               ZWRITEOPTIONS *zoptions, const char* keyfile, int cipher);
int do_encrypt_des(const char *keyfile, const char *in, int len, FILE *out);
int do_encrypt_aes(const char *keyfile, const char *in, int len, FILE *out);

int do_decrypt(const char *keyfile, int cipher);
int do_decrypt_aes(const char *keyfile);
int do_decrypt_des(const char *keyfile);


#define M_NONE            0
#define M_ZEPHYR_ENCRYPT  1
#define M_DECRYPT         2
#define M_ENCRYPT         3
#define M_RANDOMIZE       4
#define M_SETKEY          5

enum cipher_algo {
  CIPHER_DES,
  CIPHER_AES,
  NCIPHER
};

typedef struct {
  int (*encrypt)(const char *keyfile, const char *in, int len, FILE *out);
  int (*decrypt)(const char *keyfile);
} cipher_pair;

cipher_pair ciphers[NCIPHER] = {
  [CIPHER_DES] { do_encrypt_des, do_decrypt_des},
  [CIPHER_AES] { do_encrypt_aes, do_decrypt_aes},
};

static void owl_zcrypt_string_to_schedule(char *keystring, des_key_schedule *schedule) {
#ifdef HAVE_KERBEROS_IV
  des_cblock key;
#else
  des_cblock _key, *key = &_key;
#endif

  des_string_to_key(keystring, key);
  des_key_sched(key, *schedule);
}

int main(int argc, char *argv[])
{
  char *cryptspec = NULL;
  const char *keyfile;
  int cipher;
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

  while ((c = getopt(argc, argv, "ZDERSF:c:i:advqtluons:f:m")) != (char)EOF)
  {
    switch(c)
    {
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
        if (cryptspec != NULL) error = TRUE;
        cryptspec = optarg;
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
        /* Message on rest of line*/
        messageflag = TRUE;
        break;
      case '?':
        error = TRUE;
        break;
    }
    if (error || messageflag)
      break;
  }

  if (class != NULL || instance != NULL)
    zephyr = TRUE;

  if (messageflag)
  {
    zoptions.flags |= ZCRYPT_OPT_MESSAGE;
    zoptions.message = BuildArgString(argv, optind, argc);
    if (!zoptions.message)
    {
      fprintf(stderr, "Memory allocation error.\n");
      error = TRUE;
    }
  }
  else if (optind < argc)
  {
    error = TRUE;
  }

  if (mode == M_NONE)
    mode = (zephyr?M_ZEPHYR_ENCRYPT:M_ENCRYPT);

  if (mode == M_ZEPHYR_ENCRYPT && !zephyr)
    error = TRUE;

  if (!error && cryptspec == NULL && (class != NULL || instance != NULL)) {
    cryptspec = GetZephyrVarKeyFile(argv[0], class, instance);
    if(!cryptspec) {
      fprintf(stderr, "Unable to find keyfile for ");
      if(class != NULL) {
        fprintf(stderr, "-c %s ", class);
      }
      if(instance != NULL) {
        fprintf(stderr, "-i %s ", instance);
      }
      fprintf(stderr, "\n");
      exit(-1);
    }
  }

  if (error || !cryptspec)
  {
    fprintf(stderr, "Usage: %s [-Z|-D|-E|-R|-S] [-F Keyfile] [-c class] [-i instance]\n", argv[0]);
    fprintf(stderr, "       [-advqtluon] [-s signature] [-f arg] [-m message]\n");
    fprintf(stderr, "  One or more of class, instance, and keyfile must be specified.\n");
    exit(1);
  }

  cipher = ParseCryptSpec(cryptspec, &keyfile);
  if(cipher < 0) {
    fprintf(stderr, "Invalid cipher specification: %s\n", cryptspec);
    exit(1);
  }


  if (mode == M_RANDOMIZE)
  {
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
    fprintf(stderr, "Feature not yet implemented.\n");
  }
  else if (mode == M_SETKEY)
  {
    /* Set a new, user-entered key */
    char newkey[MAX_KEY];
    FILE *fkey;

    if (isatty(0))
    {
      printf("Enter new key: ");
      /* Really should read without echo!!! */
    }
    if(!fgets(newkey, MAX_KEY - 1, stdin)) {
      fprintf(stderr, "Error reading key.\n");
      return 1;
    }

    fkey = fopen(keyfile, "w");
    if (!fkey)
      fprintf(stderr, "Could not open key file for writing: %s\n", keyfile);
    else
    {
      if (fputs(newkey, fkey) != strlen(newkey) || putc('\n', fkey) != '\n')
      {
        fprintf(stderr, "Error writing to key file.\n");
        fclose(fkey);
        exit(1);
      }
      else
      {
        fclose(fkey);
        fprintf(stderr, "Key update complete.\n");
      }
    }
  }
  else
  {
    if (mode == M_ZEPHYR_ENCRYPT || mode == M_ENCRYPT)
      error = !do_encrypt((mode == M_ZEPHYR_ENCRYPT), class, instance,
                          &zoptions, keyfile, cipher);
    else
      error = !do_decrypt(keyfile, cipher);
  }

  /* Always print the **END** message if -D is specified. */
  if (mode == M_DECRYPT)
    printf("**END**\n");

  return error;
}

int ParseCryptSpec(const char *spec, const char **keyfile) {
  int cipher = CIPHER_DES;
  char *cipher_name = strdup(spec);
  char *colon = strchr(cipher_name, ':');

  *keyfile = spec;

  if (colon) {
    char *rest = strchr(spec, ':') + 1;
    while(isspace(*rest)) rest++;

    *colon-- = '\0';
    while (colon >= cipher_name && isspace(*colon)) {
      *colon = '\0';
    }

    if(strcmp(cipher_name, "AES") == 0) {
      cipher = CIPHER_AES;
      *keyfile = rest;
    } else if(strcmp(cipher_name, "DES") == 0) {
      cipher = CIPHER_DES;
      *keyfile = rest;
    }
  }

  free(cipher_name);

  return cipher;
}

/* Build a space-separated string from argv from elements between start  *
 * and end - 1.  malloc()'s the returned string. */
char *BuildArgString(char **argv, int start, int end)
{
  int len = 1;
  int i;
  char *result;

  /* Compute the length of the string.  (Plus 1 or 2) */
  for (i = start; i < end; i++)
    len += strlen(argv[i]) + 1;

  /* Allocate memory */
  result = (char *)malloc(len);
  if (result)
  {
    /* Build the string */
    char *ptr = result;
    /* Start with an empty string, in case nothing is copied. */
    *ptr = '\0';
    /* Copy the arguments */
    for (i = start; i < end; i++)
    {
      char *temp = argv[i];
      /* Add a space, if not the first argument */
      if (i != start)
        *ptr++ = ' ';
      /* Copy argv[i], leaving ptr pointing to the '\0' copied from temp */
      while ((*ptr = *temp++))
        ptr++;
    }
  }

  return result;
}

#define MAX_BUFF 258
#define MAX_SEARCH 3
/* Find the class/instance in the .crypt-table */
char *GetZephyrVarKeyFile(const char *whoami, const char *class, const char *instance)
{
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
  if (instance)
    varname[numsearch++] = g_strdup_printf("crypt-%s-%s:", (class?class:"message"), instance);
  if (class)
    varname[numsearch++] = g_strdup_printf("crypt-%s:", class);
  varname[numsearch++] = g_strdup("crypt-default:");

  /* Setup the result array, and determine string lengths */
  for (i = 0; i < numsearch; i++)
  {
    result[i][0] = '\0';
    length[i] = strlen(varname[i]);
  }

  /* Open~/.crypt-table */
  filename = g_strdup_printf("%s/.crypt-table", getenv("HOME"));
  fsearch = fopen(filename, "r");
  if (fsearch)
  {
    /* Scan file for a match */
    while (!feof(fsearch))
    {
      if (!fgets(buffer, MAX_BUFF - 3, fsearch)) break;
      for (i = 0; i < numsearch; i++)
        if (strncasecmp(varname[i], buffer, length[i]) == 0)
        {
          int j;
          for (j = length[i]; buffer[j] == ' '; j++)
            ;
          strcpy(result[i], &buffer[j]);
          if (*result[i])
            if (result[i][strlen(result[i])-1] == '\n')
              result[i][strlen(result[i])-1] = '\0';
        }
    }

    /* Pick the "best" match found */
    keyfile = NULL;
    for (i = 0; i < numsearch; i++)
      if (*result[i])
      {
        keyfile = result[i];
        break;
      }

    if (keyfile != NULL)
    {
      /* Prepare result to be returned */
      char *temp = keyfile;
      keyfile = (char *)malloc(strlen(temp) + 1);
      if (keyfile)
        strcpy(keyfile, temp);
      else
        fprintf(stderr, "Memory allocation error.\n");
    }
    fclose(fsearch);
  }
  else
    fprintf(stderr, "Could not open key table file: %s\n", filename);

  for(i = 0; i < MAX_SEARCH; i++) {
    if(varname[i] != NULL) {
      g_free(varname[i]);
    }
  }

  if(filename != NULL) {
    g_free(filename);
  }

  return keyfile;
}

static pid_t zephyrpipe_pid = 0;

/* Open a pipe to zwrite */
FILE *GetZephyrPipe(const char *class, const char *instance, const ZWRITEOPTIONS *zoptions)
{
  int fildes[2];
  pid_t pid;
  FILE *result;
  const char *argv[20];
  int argc = 0;

  if (pipe(fildes) < 0)
    return NULL;
  pid = fork();

  if (pid < 0)
  {
    /* Error: clean up */
    close(fildes[0]);
    close(fildes[1]);
    result = NULL;
  }
  else if (pid == 0)
  {
    /* Setup child process */
    argv[argc++] = "zwrite";
    argv[argc++] = "-n";     /* Always send without ping */
    if (class)
    {
      argv[argc++] = "-c";
      argv[argc++] = class;
    }
    if (instance)
    {
      argv[argc++] = "-i";
      argv[argc++] = instance;
    }
    if (zoptions->flags & ZWRITE_OPT_NOAUTH)
      argv[argc++] = "-d";
    if (zoptions->flags & ZWRITE_OPT_QUIET)
      argv[argc++] = "-q";
    if (zoptions->flags & ZWRITE_OPT_VERBOSE)
      argv[argc++] = "-v";
    if (zoptions->flags & ZWRITE_OPT_SIGNATURE)
    {
      argv[argc++] = "-s";
      argv[argc++] = zoptions->signature;
    }
    argv[argc++] = "-O";
    argv[argc++] = "crypt";
    argv[argc] = NULL;
    close(fildes[1]);
    if (fildes[0] != STDIN_FILENO)
    {
      if (dup2(fildes[0], STDIN_FILENO) != STDIN_FILENO)
        exit(0);
      close(fildes[0]);
    }
    close(fildes[0]);
    execvp(argv[0], (char **)argv);
    fprintf(stderr, "Exec error: could not run zwrite\n");
    exit(0);
  }
  else
  {
    close(fildes[0]);
    /* Create a FILE * for the zwrite pipe */
    result = (FILE *)fdopen(fildes[1], "w");
    zephyrpipe_pid = pid;
  }

  return result;
}

/* Close the pipe to zwrite */
void CloseZephyrPipe(FILE *pipe)
{
  fclose(pipe);
  waitpid(zephyrpipe_pid, NULL, 0);
  zephyrpipe_pid = 0;
}

#define BASE_CODE 70
#define LAST_CODE (BASE_CODE + 15)
#define OUTPUT_BLOCK_SIZE 16

void block_to_ascii(unsigned char *output, FILE *outfile)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    putc(((output[i] & 0xf0) >> 4) + BASE_CODE, outfile);
    putc( (output[i] & 0x0f)       + BASE_CODE, outfile);
  }
}

char *slurp_stdin(int ignoredot, int *length) {
  char *buf;
  char *inptr;

  if ((inptr = buf = (char *)malloc(MAX_RESULT)) == NULL)
  {
    fprintf(stderr, "Memory allocation error\n");
    return NULL;
  }
  while (inptr - buf < MAX_RESULT - MAX_LINE - 20)
  {
    if (fgets(inptr, MAX_LINE, stdin) == NULL)
      break;

    if (inptr[0])
    {
      if (inptr[0] == '.' && inptr[1] == '\n' && !ignoredot)
      {
        inptr[0] = '\0';
        break;
      }
      else
        inptr += strlen(inptr);
    }
    else
      break;
  }
  *length = inptr - buf;

  return buf;
}

char *GetInputBuffer(ZWRITEOPTIONS *zoptions, int *length) {
  char *buf;

  if (zoptions->flags & ZCRYPT_OPT_MESSAGE)
  {
    /* Use the -m message */
    buf = strdup(zoptions->message);
    *length = strlen(buf);
  }
  else
  {
    if (isatty(0)) {
      /* tty input, so show the "Type your message now..." message */
      if (zoptions->flags & ZCRYPT_OPT_IGNOREDOT)
        printf("Type your message now.  End with the end-of-file character.\n");
      else
        printf("Type your message now.  End with control-D or a dot on a line by itself.\n");
    } else {
      zoptions->flags |= ZCRYPT_OPT_IGNOREDOT;
    }

    buf = slurp_stdin(zoptions->flags & ZCRYPT_OPT_IGNOREDOT, length);
  }
  return buf;
}

char *read_keystring(const char *keyfile) {
  char *keystring;
  FILE *fkey = fopen(keyfile, "r");
  if(!fkey) {
    fprintf(stderr, "Unable to open keyfile %s\n", keyfile);
    return NULL;
  }
  keystring = malloc(MAX_KEY);
  if(!fgets(keystring, MAX_KEY-1, fkey)) {
    fprintf(stderr, "Unable to read from keyfile: %s\n", keyfile);
    free(keystring);
    keystring = NULL;
  }
  fclose(fkey);
  return keystring;
}

/* Encrypt stdin, with prompt if isatty, and send to stdout, or to zwrite
   if zephyr is set. */
int do_encrypt(int zephyr, const char *class, const char *instance,
               ZWRITEOPTIONS *zoptions, const char *keyfile, int cipher)
{
  FILE *outfile = stdout;
  char *inbuff = NULL;
  int buflen;
  int out = TRUE;

  inbuff = GetInputBuffer(zoptions, &buflen);

  if(!inbuff) {
    fprintf(stderr, "Error reading zcrypt input!\n");
    return FALSE;
  }

  if (zephyr) {
    outfile = GetZephyrPipe(class, instance, zoptions);
    if (!outfile)
    {
      fprintf(stderr, "Could not run zwrite\n");
      if (inbuff)
        free(inbuff);
      return FALSE;
    }
  }

  out = ciphers[cipher].encrypt(keyfile, inbuff, buflen, outfile);

  if (zephyr)
    CloseZephyrPipe(outfile);

  free(inbuff);
  return out;
}

int do_encrypt_des(const char *keyfile, const char *in, int length, FILE *outfile)
{
  des_key_schedule schedule;
  unsigned char input[8], output[8];
  const char *inptr;
  int num_blocks, last_block_size;
  char *keystring;
  int size;

  keystring = read_keystring(keyfile);
  if(!keystring) {
    return FALSE;
  }

  owl_zcrypt_string_to_schedule(keystring, &schedule);
  free(keystring);

  inptr = in;
  num_blocks = (length + 7) / 8;
  last_block_size = ((length + 7) % 8) + 1;

  /* Encrypt the input (inbuff or stdin) and send it to outfile */
  while (TRUE)
  {
    /* Get 8 bytes from buffer */
    if (num_blocks > 1)
    {
      size = 8;
      memcpy(input, inptr, size);
      inptr += 8;
      num_blocks--;
    }
    else if (num_blocks == 1)
    {
      size = last_block_size;
      memcpy(input, inptr, size);
      num_blocks--;
    }
    else
      size = 0;

    /* Check for EOF and pad the string to 8 chars, if needed */
    if (size == 0)
      break;
    if (size < 8)
      memset(input + size, 0, 8 - size);

    /* Encrypt and output the block */
    des_ecb_encrypt(&input, &output, schedule, TRUE);
    block_to_ascii(output, outfile);

    if (size < 8)
      break;
  }

  putc('\n', outfile);

  return TRUE;
}

int do_encrypt_aes(const char *keyfile, const char *in, int length, FILE *outfile)
{
  char *out;
  int err, status;
  const char *argv[] = {
    "gpg",
    "--symmetric",
    "--batch",
    "--quiet",
    "--no-use-agent",
    "--armor",
    "--cipher-algo", "AES",
    "--passphrase-file", keyfile,
    NULL
  };
  err = call_filter("gpg", argv, in, &out, &status);
  if(err || status) {
    if(out) g_free(out);
    return FALSE;
  }
  fwrite(out, strlen(out), 1, outfile);
  g_free(out);
  return TRUE;
}

/* Read a half-byte from stdin, skipping invalid characters.  Returns -1
   if at EOF or file error */
int read_ascii_nybble(void)
{
  char c;

  while (TRUE)
  {
    if (fread(&c, 1, 1, stdin) == 0)
      return -1;
    else if (c >= BASE_CODE && c <= LAST_CODE)
      return c - BASE_CODE;
  }
}

/* Read both halves of the byte and return the single byte.  Returns -1
   if at EOF or file error. */
int read_ascii_byte(void)
{
  int c1, c2;
  c1 = read_ascii_nybble();
  if (c1 >= 0)
  {
    c2 = read_ascii_nybble();
    if (c2 >= 0)
    {
      return c1 * 0x10 + c2;
    }
  }
  return -1;
}

/* Read an 8-byte DES block from stdin */
int read_ascii_block(unsigned char *input)
{
  int c;

  int i;
  for (i = 0; i < 8; i++)
  {
    c = read_ascii_byte();
    if (c < 0)
      return FALSE;

    input[i] = c;
  }

  return TRUE;
}

/* Decrypt stdin */
int do_decrypt(const char *keyfile, int cipher)
{
  return ciphers[cipher].decrypt(keyfile);
}

int do_decrypt_aes(const char *keyfile) {
  char *in, *out;
  int length;
  const char *argv[] = {
    "gpg",
    "--decrypt",
    "--batch",
    "--no-use-agent",
    "--quiet",
    "--passphrase-file", keyfile,
    NULL
  };
  int err, status;

  in = slurp_stdin(TRUE, &length);
  if(!in) return FALSE;

  err = call_filter("gpg", argv, in, &out, &status);
  if(err || status) {
    if(out) g_free(out);
    return FALSE;
  }
  fwrite(out, strlen(out), 1, stdout);
  g_free(out);

  return TRUE;
}

int do_decrypt_des(const char *keyfile) {
  des_key_schedule schedule;
  unsigned char input[8], output[8];
  char tmp[9];
  char *keystring;

  /*
    DES decrypts 8 bytes at a time. We copy those over into the 9-byte
    'tmp', which has the final byte zeroed, to ensure that we always
    have a NULL-terminated string we can call printf/strlen on.

    We don't pass 'tmp' to des_ecb_encrypt directly, because it's
    prototyped as taking 'unsigned char[8]', and this avoids a stupid
    cast.

    We zero 'tmp' entirely, not just the final byte, in case there are
    no input blocks.
  */
  memset(tmp, 0, sizeof tmp);

  keystring = read_keystring(keyfile);
  if(!keystring) return FALSE;

  owl_zcrypt_string_to_schedule(keystring, &schedule);

  free(keystring);

  while (read_ascii_block(input))
  {
    des_ecb_encrypt(&input, &output, schedule, FALSE);
    memcpy(tmp, output, 8);
    printf("%s", tmp);
  }

  if (!tmp[0] || tmp[strlen(tmp) - 1] != '\n')
      printf("\n");
  return TRUE;
}
