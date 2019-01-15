/* Sophia Fondell
A Basic Shell
11/3/18 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BIG_NUM 32767

// Check whether arg char is a ~special character~
int sp(int val) {
  switch (val) {
    // |
  case 124: return 1;
    // >
  case 62: return 2;
    // <
  case 60: return 3;
    // &
  case 38: return 4;
    // ;
  case 59: return 5;
    // Not special character
  default: return 0;
  }
}

// Finds the length of a string with multiple null characters in it
// Excludes null terminator as a normal strlen() function would
// bc we always know that the last character is going to be null
int len(char* start, char* stop, char* linebuf) {
	int length = 0;
	// Initialize iterator pointer
	char* i = start;
	while (i != stop) {
		length++;
		i++;
	}
	return length + 1;
}

// Function to count the number of strings separated by null characters in a subarray
int stringct(char* start, char* stop, char* linebuf) {
	// Assumes that start is the special character we're starting at
	// + 2 moves the pointer to the first non null char in subarray bc
	// everything separated by exactly one null character bc of parser
	char* i = start;
	int count = 0;
	// Whether the last character we iterated over is a character or not
	int charlst = 0;
	while (i != stop) {
		// Current character is null
		if (*i == NULL) {
			charlst = 0;
			i++;
		}
		// Current character is not null
		else {
			// Last char was null so this is a new string in subarr
			if (!charlst) {
				charlst = 1;
				count++;
				i++;
			}
			// Current character is part of string we've already accounted for
			else {
				charlst = 1;
				i++;
			}
		}
	}
	return count;
}

// Function to count the number of spchars in a string w multiple null characters
int count_sp(char* start, char* stop, char* linebuf) {
	int spchars = 0;
	char* i = start;
	while (i != stop) {
		if (sp(*i)) {
			spchars++;
			i++;
		}
		else {
			i++;
		}
	}
	return spchars;
}


// Function to return an argv array of pointers to cmd and cmd args in subarr
char** make_argv(char* start, char* stop, char* linebuf, int stringct) {
	// Declare argv array of pointers
	char** argv = malloc(sizeof(char*) * (stringct + 1));
	// Initialize iterator pointer (w same reasoning as above)
	char* i = start;
	// Initialize iterator variable for argv
	int j = 0;
	// Whether last character we iterated over is a character or not
	int charlst = 0;
	// While i does not reference the same addr as stop and we haven't accounted
	// for all strings in the array yet
	while (i != stop && j < stringct) {
		// Current character is null
		if (*i == NULL) {
			charlst = 0;
			i++;
		}
		// Current character is not null
		else {
			// Last character was null so this is a new string we need addr for
			if (!charlst) {
				charlst = 1;
				argv[j] = i;
				j++;
				i++;
			}
			// Last character was not null so we've already found addr for start of string
			else {
				charlst = 1;
				i++;
			}
		}
	}
	argv[stringct] = NULL;
	return argv;
}

int executor(char* ptrarr[], char* linebuf, int pipes, char* lbstop) {	
	// Status of child process
	int status;
	// Process ID
	int pid;
	// Array of parent to child fds
	int pc[2];
	// Array of child to parent fds
	int cp[2];
	// First command bool
	int first = 1;
	// Fork the current process
	pid = fork();
	// Parent
	if (pid > 0) {
		// Wait for the status of the child process to change
		waitpid(pid, &status, 0);
	}
	// Child
	else {
		// Parent wrote to child bool
		int pwrite = 0;
		// Iterator var for ptrarr
		int piped = 0;
		// Status of grandchild process
		int status2;
		// If no special characters in linebuf at all
		if (!count_sp(&linebuf[0], lbstop, linebuf)) {
			// Get string count of cmd and any args in subarr
			int strct = stringct(&linebuf[0], lbstop, linebuf);
			// Pass arr, count and pointers into argv function
			char** argv = make_argv(&linebuf[0], lbstop, linebuf, strct);
			// Pass argv array into execvp
			int errno = execvp(argv[0], argv);
			// Check return value of execvp
			if (errno < 0) {
				perror("error");
			}
		}
		else if (pipes > 0) {
			// ENTER LOOP IF WE HAVE STRINGS OF COMMANDS WITH PIPES BETWEEN THEM TO PARSER
			while (pipes) {
				// Creating a pipe before the process forks
				// pc[0] = Parent to child reading fd
				// pc[1] = Parent to child writing fd
				// cp[0] = Child to parent reading fd
				// cp[1] = Child to parent writing fd
				pipe(cp);
				pipe(pc);

				// Fork the current process
				int pid2 = fork();
				// Parent process
				if (pid2 > 0) {
					// Wait for status of child to change
					waitpid(pid2, &status2, 0);
					// What kind of input are we getting and what do we need to do with it?
					// Make and iterate through subarr to figure that out
					// Find the size of characters between the 2 pipes ??
					int size = (long unsigned int) ptrarr[piped + 1] - (long unsigned int) ptrarr[piped];
					// Length of array between pipes because sizeof(char) = 1 
					char subarr[size];
					// Add characters from original linebuf array to subarr for execution
					int i;
					// Copy characters from linebuf to subarr within specified indices
					for (i = 0; i < size; i++) {
						// Dereference specified values of linebuf and assign them to subarr
						subarr[i] = *(ptrarr[piped] + i);
					}
					// Iterator var for subarr
					int j;
					// Iterate through subarr and look for special characters
					for (j = 0; j < size; j++) {
						// If we find a redirection character:
						// Redirect stdin of cmd to input file
						// Ex: sp(val) == 3 -> "wc < hi.txt"
						// OR
						// Redirect stdout of command to input file
						// Ex: sp(val) == 2 -> "ps aux > hi.txt"
						if ((sp(subarr[j]) == 2) || (sp(subarr[j]) == 3)) {
							// Get fd of child -> parent reading pipe
							int fd = dup(cp[0]);
							// Declare buffer in which to store bytes from pipe
							char buf[BIG_NUM];
							// Read bytes from file into buffer and assign # read bytes to val
							read(fd, buf, BIG_NUM);
							// Write bytes to parent -> child writing pipe
							write(pc[1], buf, BIG_NUM);
							// Parent wrote to child
							pwrite = 1;
						}
						else {
							pwrite = 0;
						}
					}
				}
				// Child process
				else {
					// Find the size of characters between the 2 pipes ??
					int size = (long unsigned int) ptrarr[piped + 1] - (long unsigned int) ptrarr[piped];
					// Length of array between pipes because sizeof(char) = 1 
					char subarr[size];
					// Add characters from original linebuf array to subarr for execution
					int i;
					// Copy characters from linebuf to subarr within specified indices
					for (i = 0; i < size; i++) {
						// Dereference specified values of linebuf and assign them to subarr
						subarr[i] = *(ptrarr[piped] + i);
					}
					// CHECK FOR SP CHARACTERS AT ALL
					// If no special characters in linebuf
					if (!count_sp(&subarr[0], &subarr[size - 1], subarr)) {
						// Nothing has been piped yet
						if (piped == 0) {
							// Get string count of cmd and any args in subarr
							int strct = stringct(&linebuf[0], lbstop, linebuf);
							// Pass arr, count and pointers into argv function
							char** argv = make_argv(&linebuf[0], lbstop, linebuf, strct);
							// Close stdout
							close(1);
							// Sets child to parent writing fd
							dup(cp[1]);
							// Pass argv array into execvp
							int errno = execvp(argv[0], argv);
							// Check return value of execvp
							if (errno < 0) {
								perror("error");
							}
							// Increment piped bc we just piped something
							piped++;
							// Decrement number of pipes we got left
							pipes--;
						}
						// Things have been piped so we gotta read from pipe
						else {
							// close stdin
							close(0);
							// replace w parent to child reading
							dup(pc[0]);
							close(pc[1]);
							// Get string count
							int strct = stringct(&linebuf[0], lbstop, linebuf);
							// Pass arr, count and pointers into argv function
							char** argv = make_argv(&linebuf[0], lbstop, linebuf, strct);
							// Close stdout
							close(1);
							// Sets child to parent writing fd
							dup(cp[1]);
							// Pass argv array into execvp
							int errno = execvp(argv[0], argv);
							// Check return value of execvp
							if (errno < 0) {
								perror("error");
							}
							// Increment piped bc we just piped something
							piped++;
							// Decrement number of pipes we got left
							pipes--;
						}
					}
					// Iterator var for subarr
					int j;
					// Iterate through subarr and look for special characters
					for (j = 0; j < size; j++) {
						// If we find a redirection character
						if (sp(subarr[j]) == 2 || sp(subarr[j]) == 3) {
							// Redirect stdin of cmd to input file
							// Ex: sp(val) == 3 -> "wc < hi.txt"
							if (sp(subarr[j] == 3)) {
								// If parent already wrote, run first half
								if (pwrite) {
									// Close stdout fd for process
									close(1);
									// Sets child to parent writing fd/pipe
									dup(cp[1]);
									// Get pointers to pass into argv functions
									char* end = &subarr[j];
									char* start = &subarr[0];
									// Get string count in first half of subarr
									int strct = stringct(start, end, subarr);
									// Pass arr, count and pointers into argv fnc
									char** argv = make_argv(start, end, subarr, strct);
									// Pass argv array into execvp
									int errno = execvp(argv[0], argv);
									// Check to make sure execvp didn't fail
									if (errno < 0) {
										perror("error");
									}
								}
								// If parent hasnt written, open file
								else {
									// Close stdout fd for process
									close(1);
									// Sets child to parent writing fd/pipe
									dup(cp[1]);
									// Declare fd int of file we want to open
									int fd;
									// Increment j by 2 so we can get the filename
									j = j + 2;
									// Declare file pointer
									FILE *fp;
									// Open the file specified at subarr[j] as read-only
									fp = fopen(&subarr[j], "r");
									// Get the file descriptor of the opened file
									fd = fileno(fp);
									// Declare buffer in which to store text from file
									char filebuf[BIG_NUM];
									// Declare long int of the number of bytes read from file
									long int read_bytes = 0;
									// Read bytes from file into buffer and assign # read bytes to val
									read_bytes = read(fd, filebuf, BIG_NUM);
									// Close the opened file's fd
									close(fd);
									// Write the contents of the filebuffer to the child to parent writing pipe
									// So that we can retrieve the file contents in the parent process
									read_bytes = write(cp[1], filebuf, BIG_NUM);
									// Close the child to parent writing pipe
									close(cp[1]);
									// Process is done so it can die
									exit(0);
								}
							}
							// Redirect stdout of cmd to output file
							// Ex: sp(val) == 2 -> "ps aux > hi.txt"
							else {
								if (pwrite) {
									// Declare fd int of file we want to open
									int fd;
									// Increment j by 2 so we can get the filename
									j = j + 2;
									// Declare file pointer
									FILE *fp;
									// Open the file specified at subarr[j] as read-only
									fp = fopen(&subarr[j], "w+");
									// Get the file descriptor of the opened file
									fd = fileno(fp);
									// Declare buffer in which to store text from file
									char filebuf[BIG_NUM];
									// Declare long int of the number of bytes read from file
									long int read_bytes = 0;
									// Read bytes from file into buffer and assign # read bytes to val
									read_bytes = read(pc[0], filebuf, BIG_NUM);
									// Close the opened file's fd
									close(pc[0]);
									// Write the contents of the filebuffer to the child to parent writing pipe
									// So that we can retrieve the file contents in the parent process
									read_bytes = write(fd, filebuf, BIG_NUM);
									// Process is done so it can die
									exit(0);
								}
								else {
									// Close stdout fd for process
									close(1);
									// Sets child to parent writing fd/pipe
									dup(cp[1]);
									// Get pointers to pass into argv functions
									char* end = &subarr[j];
									char* start = &subarr[0];
									// Get string count in first half of subarr
									int strct = stringct(start, end, subarr);
									// Pass arr, count and pointers into argv fnc
									char** argv = make_argv(start, end, subarr, strct);
									// Pass argv array into execvp
									int errno = execvp(argv[0], argv);
									// Check to make sure execvp didn't fail
									if (errno < 0) {
										perror("error");
									}
								}
							}
						}
					}
					// No special characters in this subarray so that means there's
					// just a command to execute
					if (pwrite) {
						close(0);
						dup(pc[0]);
					}
					// Close stdout fd
					close(1);
					// Sets child to parent writing fd/pipe
					dup(cp[1]);
					// Initialize pointers to pass into argv functions
					char* end = &subarr[size - 1];
					char* start = &subarr[0];
					// Get string count of cmd and any args in subarr
					int strct = stringct(start, end, subarr);
					// Pass arr, count and pointers into argv function
					char** argv = make_argv(start, end, subarr, strct);
					// Pass argv array into execvp
					int errno = execvp(argv[0], argv);
					// Check return value of execvp
					if (errno < 0) {
						perror("error");
					}
				}
			}
		}

		// JUST ONE STRING OF COMMANDS TO PARSE NO PIPES
		else {
			// Creating a pipe before the process forks
			pipe(cp);
			pipe(pc);
			// Length of linebuf
			int size = len(&linebuf[0], lbstop, linebuf);
			// fork the current process
			int pid2 = fork();
			// Parent process
			if (pid2 > 0) {
				// Wait for status of child to change
				waitpid(pid2, &status2, 0);
				puts("staywoke");
				// Iterator var for linebuf
				int j;
				// Iterate through linebuf and look for special characters
				for (j = 0; j < size; j++) {
					// If we find a redirection character:
					// Redirect stdin of cmd to input file
					// Ex: sp(val) == 3 -> "wc < hi.txt"
					// OR
					// Redirect stdout of command to input file
					// Ex: sp(val) == 2 -> "ps aux > hi.txt"
					if ((sp(linebuf[j]) == 2) || (sp(linebuf[j]) == 3)) {
						if (sp(linebuf[j]) == 2) {
							// Get fd of child -> parent reading pipe
							//int fd = dup(cp[0]);
							FILE *fp;
							// Open the file specified at subarr[j] as read-only
							fp = fopen(&linebuf[j + 2], "w+");
							// Get the file descriptor of the opened file
							int fd1 = fileno(fp);	
							// Declare buffer in which to store bytes from pipe
							char buf[BIG_NUM];
							// Read bytes from file into buffer and assign # read bytes to val
							int reead = read(cp[0], buf, BIG_NUM);
							printf("%d\n", reead);
							// Write bytes to parent -> child writing pipe				
							reead = write(fd1, buf, reead);
							fclose(fp);
							exit(0);
						} 
						else {
							puts("else");
							// Get fd of child -> parent reading pipe
							close(0);

							int fd = dup(cp[0]);
							printf("%d\n", fd);
							close(cp[1]);
							// Declare buffer in which to store bytes from pipe
							//char buf[BIG_NUM];
							// Get pointers to pass into argv functions
							char* end = &(linebuf[j - 1]);
							char* start = &(linebuf[0]);
							// Get string count in first half of linebuf
							int strct = stringct(start, end, linebuf);
							// Pass arr, count and pointers into argv fnc
							char** argv = make_argv(start, end, linebuf, strct);
							printf("%s %s %s", argv[0], argv[1], argv[2]);
							// Pass argv array into execvp
							puts("trying");
							int errno = execvp(argv[0], argv);
							// Check to make sure execvp didn't fail
							
								perror("aaaaaaaaaaaaaaaaaahhhhhhhhhhh");
							
						}	
					}
					else {

						pwrite = 0;
					}
				}
				/*// No special characters only command, do not pass to child
				// Get pointers to pass into argv functions
				char* end = &linebuf[size - 1];
				char* start = &linebuf[0];
				// Get string count in first half of linebuf
				int strct = stringct(start, end, linebuf);
				// Pass arr, count and pointers into argv fnc
				char** argv = make_argv(start, end, linebuf, strct);
				// Pass argv array into execvp
				int errno = execvp(argv[0], argv);
				// Check to make sure execvp didn't fail
				if (errno < 0) {
					perror("error");
				}*/
			}
			// Child process
			else {
				// Iterator var for linebuf
				int j;
				puts("started loop");
				// Iterate though linebuf and look for special characters
				for (j = 0; j < size; j++) {
					// Redirect stdin of cmd to input file
					// Ex: sp(val) == 3 -> "wc < hi.txt"
					if (sp(linebuf[j]) == 3) {
						puts("send help");
						// Close stdout fd for process
						//close(1);
						// Sets child to parent writing fd/pipe
						dup(cp[1]);
						// Declare fd int of file we want to open
						int fd;
						// Increment j by 2 so we can get the filename
						j = j + 2;
						// Declare file pointer
						FILE *fp;
						// Open the file specified at subarr[j] as read-only
						fp = fopen(&linebuf[j], "r");
						// Get the file descriptor of the opened file
						fd = fileno(fp);
						// Declare buffer in which to store text from file
						char filebuf[BIG_NUM];
						// Declare long int of the number of bytes read from file
						long int read_bytes = 0;
						// Read bytes from file into buffer and assign # read bytes to val
						read_bytes = read(fd, filebuf, BIG_NUM);
						printf("read: %d\n", read_bytes);
						// Close the opened file's fd
						close(fd);
						// Write the contents of the filebuffer to the child to parent writing pipe
						// So that we can retrieve the file contents in the parent process
						read_bytes = write(cp[1], filebuf, read_bytes);
						printf("write: %d\n", read_bytes);
						// Close the child to parent writing pipe
						close(cp[1]);
						// Process is done so it can die
						exit(0);
					}
					// Redirect stdout of cmd to output file
					// Ex: sp(val) == 2 -> "ps aux > hi.txt"
					else if (sp(linebuf[j]) == 2) {
						
						// Get pointers to pass into argv functions
						char* end = &linebuf[j];
						char* start = &linebuf[0];
						// Get string count in first half of linebuf
						int strct = stringct(start, end, linebuf);
						// Pass arr, count and into argv fnc
						char** argv = make_argv(start, end, linebuf, strct);
						// Pass argv array into execvp
						//printf("Execvp(%s , %s, %s )",argv[0], argv[1], argv[2]);
						// Close stdout fd for process
						close(1);
						// Sets child to parent writing fd/pipe
						dup(cp[1]);
						int errno = execvp(argv[0], argv);
						// Check to make sure execvp didn't fail
						if (errno < 0) {
							perror("error");
						}
					}
				}
			}
		}
	}
}



