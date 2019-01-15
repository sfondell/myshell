/* Sophia Fondell
A Basic Shell - parser.c
11/3/18 */

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include "execute.h"

/* Special Characters
1 = |
2 = >
3 = <
4 = &
5 = ;
*/

// Count the number of pipes in a string
int count_pipes(char* linebuf) {
  // Index
  int i;
  int count = 0;
  // Iterate through linebuf
  for (i = 0; i < strlen(linebuf); i++) {
    if (sp(linebuf[i]) == 1) {
      count++;
    }
  }
  return count;
}

// Parse linebuf
char* parse(char* ptrarr[], char* linebuf) {
  // linebuf iterator
  int i;
  int siz  = strlen(linebuf);
  for (i = 0; i < siz; i++) {
    if (linebuf[i] == ' ') {
      linebuf[i] = '\0';
    }
  }


  /*// Initialize new linebuffer and copy only non-space characters
  // Needs to be AT LEAST as big as linebuf - 1
  char* linebuf2 = malloc(sizeof(char) * strlen(linebuf - 1));
  // linebuf2 index
  int k = 0;
  // Initializing i w starting index
  i = 0;
  // Flag for 1 null between chars
  int spflag = 0;
  while (linebuf[i] != '\0') {
    // Current character is not a space or a newline character
    if ((linebuf[i] != ' ') && (linebuf[i] != '\n')) {
      linebuf2[k] = linebuf[i];
      k++;
      i++;
      // The last replaced character is not a space
      spflag = 0;
    }
    // Current character is a space
    else {
      // Have we already accounted for this space in new array only once?
      if (!spflag) {
       linebuf2[k] = '\0';
       k++;
       i++;
       spflag = 1;
      }
      else {
       i++;
      }
    }
  }*/
  char* lbstop = &linebuf[i - 1];
  // For usage in len() function
  char* start = &linebuf[0];
  // ptrarr index
  int j = 0;
  // Iterate through new linebuf and put pointers to pipe characters in ptrarr
  for (i = 0; i < len(start, lbstop, linebuf); i++) {
    if (sp(linebuf[i]) == 1) {
      ptrarr[j] = &linebuf[i];
      j++;
    }
  }
  return lbstop;  
}

int main(int argc, char *argv[]) {
  // Initialize linebuf
  char linebuf[LINE_MAX];
    while(1) {
      // Command prompt
      printf("myshell>");
      // Read commands from stdin
      fgets(linebuf, LINE_MAX, stdin);
      // Replace newline char at end of string w null
      if (linebuf[strlen(linebuf) - 1] == '\n') {
        linebuf[strlen(linebuf) - 1] = '\0';
      }
      // Get the number of pipes in the string
      int pipes = count_pipes(linebuf);
      // Declare array of pointers to each pipe in the string of commands
      char** ptrarr = malloc(sizeof(char*) * pipes);
      // Call main parsing function
      // Return line buffer that has spaces replaced w null & only one null character between
      // each command/argument/flag
      char* lbstop = parse(ptrarr, linebuf);
      // Call main execute function
      int ret = executor(ptrarr, linebuf, pipes, lbstop);
    }
  return 0;
}
