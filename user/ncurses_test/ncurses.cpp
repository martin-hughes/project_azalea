#include <stdio.h>
#include <string.h>
#include <ncurses/ncurses.h>
#include <azalea/azalea.h>

extern "C" int main();


#define SC_DEBUG_MSG(string) \
  syscall_debug_output((string), strlen((string)) )

int main()
{
  SC_DEBUG_MSG("ncurses test program\n");
  initscr();
  SC_DEBUG_MSG("init\n");
  printw("Hello World !!!");
  SC_DEBUG_MSG("Printed\n");
  SC_DEBUG_MSG("Refreshed\n");
  keypad(stdscr, TRUE);
  noecho();
  cbreak();
  refresh();
  //notimeout(stdscr, TRUE);

  nodelay(stdscr, FALSE);
  char buf[50];
  int k = -1;
  int i = 0;
  while(i < 5)
  {
    k = wgetch(stdscr);
    if (k != -1)
    {
      snprintf(buf, 50, "Key pressed: %d\n", k);
      SC_DEBUG_MSG(buf);
      i++;
    }
  }
  SC_DEBUG_MSG("Getch() returned\n");
  endwin();
  SC_DEBUG_MSG("Finished\n");

  return 0;
}
