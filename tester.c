#include "owl.h"
#include <unistd.h>
#include <stdlib.h>

static const char fileIdent[] = "$Id$";

void screeninit() {
  char buff[1024];
  
  sprintf(buff, "TERMINFO=%s", TERMINFO);
  putenv(buff);

  initscr();
  start_color();
  /* cbreak(); */
  raw();
  noecho();
  intrflush(stdscr,FALSE);
  keypad(stdscr,TRUE);
  nodelay(stdscr,1);
  clear();
  refresh();
  meta(stdscr, TRUE);
}

void test1() {
  int j;
  owl_editwin e;

  screeninit();

  owl_editwin_init(&e, stdscr, LINES, COLS, OWL_EDITWIN_STYLE_MULTILINE);
  /* owl_editwin_set_locktext(&e, "Here is some locktext:\n");*/
  doupdate();
  while (1) {
    usleep(50);

    j=getch();

    if (j==ERR) continue;

    if (j==3) break;

    if (j==27) {
      j=getch();
      if (j==ERR) continue;
      owl_editwin_process_char(&e, j);
      doupdate();
    } else {
      owl_editwin_process_char(&e, j);
      doupdate();
    }
  }
  endwin();
  printf("Had:\n%s", owl_editwin_get_text(&e));
}

void test2(char *in) {
  owl_fmtext t;

  screeninit();

  owl_fmtext_init_null(&t);
  owl_fmtext_append_ztext(&t, in);
  owl_fmtext_curs_waddstr(&t, stdscr);
  wrefresh(stdscr);
  sleep(5000);
  endwin();
}

void test3() {
  ZNotice_t *n;

  printf("%i\n", sizeof(n->z_uid.zuid_addr));
  /* gethostbyaddr((char *) &(n->z_uid.zuid_addr), sizeof(n->z_uid.zuid_addr), AF_INET); */
}

void colorinfo() {
  char buff[1024];
  
  screeninit();
  sprintf(buff, "Have %i COLOR_PAIRS\n", COLOR_PAIRS);
  addstr(buff);
  refresh();
  sleep(10);
  endwin();  
}

void test4() {
  int j;
  char buff[1024];

  screeninit();
  
  while (1) {
    usleep(100);

    j=getch();

    if (j==ERR) continue;

    if (j==3) break;
    sprintf(buff, "%o\n", j);
    addstr(buff);
  }
  endwin();
}

void test_keypress() {
  int j, rev;
  char buff[1024], buff2[64];

  screeninit();
  
  while (1) {
    usleep(100);

    j=wgetch(stdscr);

    if (j==ERR) continue;

    if (j==3) break;
    if (0 == owl_keypress_tostring(j, 0, buff2, 1000)) {
      rev = owl_keypress_fromstring(buff2);
      sprintf(buff, "%s : 0x%x 0%o %d %d %s\n", buff2, j, j, j, rev,
	      (j==rev?"matches":"*** WARNING: Does Not Reverse"));
    } else {
      sprintf(buff, "UNKNOWN : 0x%x 0%o %d\n", j, j, j);
    }
      addstr(buff);
  }
  endwin();
}


int main(int argc, char **argv, char **env) {
  int numfailures=0;
  if (argc==2 && 0==strcmp(argv[1],"reg")) {
    numfailures += owl_dict_regtest();
    numfailures += owl_variable_regtest();
    if (numfailures) {
      fprintf(stderr, "*** WARNING: %d failures total\n", numfailures);
    }
    return(numfailures);
  } else if (argc==2 && 0==strcmp(argv[1],"test1")) {
    test1();
  } else if (argc==2 && 0==strcmp(argv[1],"colorinfo")) {
    colorinfo();
  } else if (argc==2 && 0==strcmp(argv[1],"test4")) {
    test4();
  } else if (argc==2 && 0==strcmp(argv[1],"keypress")) {
    test_keypress();
  } else {
    fprintf(stderr, "No test specified.  Current options are: reg test1\n");
  }
  return(0);
}

