#define _POSIX_C_SOURCE 200809L
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#define _GNU_SOURCE

// Functions.
void removeLine(char[]);
void printArguments(char*[], int);
void freeArgs(char*[]);
void freeDups(char*[]);

int main(int argc, char **argv){
  
  // --- Initialization of variables ---
  
  // Input variables.
  char command[128];
  char* arguments[512] = {NULL};
  
  // Environment variables.
  char* ps1 = "";
  if(getenv("PS1")) ps1 = getenv("PS1");
  char* ifs = " \t\n";
  if(getenv("IFS")) ifs = getenv("IFS");
  char* home = getenv("HOME");

  // Expansion flags.
  int processExpansion = 0;
  int exitExpansion = 0;
  int backgroundExpansion = 0;
  int homeExpansion = 0;
  int noExpansion = 0;

  // Process variables.
  pid_t pid;
  pid_t sPid;
  char* holdPid;

  // Tokenization variables.
  char newArg[128];
  char newArg2[128];
  char* dupArg[256];
  char* dupArg2[256];
  int x;
  char *arg;
  int j;
  int noPost;

  // Built-in variables.
  long exitStatus;
  char exitNum[20];
  char *end;
  int dir;

  // Execution variables.
  int statusExit = 0;
  int backgroundMode;
  int builtIn;

  // Redirection variables.
  int inFlag;
  int outFlag;
  char fileStdin[512];
  char fileStdout[512];


  // Program loop.
  for (;;) {

    // Clearing memory of variables.
    memset(command, 0, 128);
    freeArgs(arguments);

    // Initialize.
    builtIn = 0;
    backgroundMode = 0;
    inFlag = 0;
    outFlag = 0;
    exitStatus = 0;

    // Getting input.
    fprintf(stderr, "%s", ps1);
    fgets(command, 128, stdin);
    //getline(&command, &commandSize, stdin);

    // Handles blank input.
    if (command[0] == '\n') continue;
    if (command[0] == ' ') continue;

    // Remove the '\n' from end of command.
    removeLine(command);


    // -------------Tokenization loop. ------------------------
    
    // Creating first argument token, and x to count args.
    arg = strtok(command, ifs);
    x = 0;

    // Loop keeps going until last arg is tokenized.
    while(arg) {
      
      // Free memory.
      memset(newArg, 0, sizeof newArg);
      memset(newArg2, 0, 128);
      freeDups(dupArg);
      freeDups(dupArg2);
      
      // Reset flags.
      homeExpansion = 0;
      processExpansion = 0;
      exitExpansion = 0;
      backgroundExpansion = 0;
      noExpansion = 0;

      // Breaks parsing loop if argument starts with comment or tab.
      if (arg[0] == '#') break;
      if (arg[0] == '\t') break;
      
      // Checking for redirection characters at beginning.
      if (arg[0] == '<' && strlen(arg) == 1){
        inFlag = 1;
      } else if (arg[0] == '>' && strlen(arg) == 1){
        outFlag = 1;
      }

      // Post-expansion index pointer.
      j = 0;
      noPost = 0;
      
      // Loop going through each character of tokenized args.
      for (int i = 0; i < strlen(arg); i++){

        // Checking for "$$" expansion.
        if (arg[i] == '$' && arg[i + 1] == '$'){
          processExpansion = 1;
          noExpansion = 1;
          dupArg[0] = strdup(newArg);
          
          // Break loop if expansion occurs at end of argument.
          if (i == (strlen(arg) - 2)) {
            noPost = 1;
            break;
          }
          i++;
          continue;
        }
        
        // Checking for "$?" expansion. 
        else if (arg[i] == '$' && arg[i + 1] == '?'){
          exitExpansion = 1;
          noExpansion = 1;
          dupArg[0] = strdup(newArg);
          if (i == (strlen(arg) - 2)){
            noPost = 1;
            break;
          }
          i++;
          continue;
        }
        
        // Checking for "$!" expansion.
        else if (arg[i] == '$' && arg[i + 1] == '!'){
          backgroundExpansion = 1;
          noExpansion = 1;
          dupArg[0] = strdup(newArg);
          break;
        }
        
        // Checking for "~/" expansion.
        else if (arg[i] == '~' && arg[i + 1] == '/'){
          homeExpansion = 1;
          noExpansion = 1;
          continue;          
        
        // Check for current expansions to build 2nd half.
        } else {

          // Part of argument after '~/' expansion.
          if (homeExpansion == 1){
            newArg2[j] = arg[i];
            j++;
            // Reached end of arg, break loop.
            if (i == strlen(arg)-1){
              dupArg[0] = strdup(newArg2);
              break;
            }
            
          // Part of argument after '$$' expansion.  
          } else if (processExpansion == 1){
            newArg2[j] = arg[i];
            j++;
            // Reached end of arg, break loop.
            if (i == strlen(arg)-1){
              dupArg2[0] = strdup(newArg2);
              break;
            }
            
          // Part of argument after '$?' expansion.
          } else if (exitExpansion == 1){
            newArg2[j] = arg[i];
            j++;
            // Reached end of arg, break loop.
            if (i == strlen(arg)-1){
              dupArg2[0] = strdup(newArg2);
              break;
            }
          
          // No expansions, copy original character over.
          } else {
            newArg[i] = arg[i];
          }
        }
      }


      // Handling '$$' variable expansion.
      if (processExpansion == 1){
        pid = getpid();
        holdPid = calloc(16, sizeof(char));
        sprintf(holdPid, "%d", pid);
        
        if (noPost == 1){
          sprintf(dupArg[0], "%s%s", dupArg[0], holdPid);
        } else {          
          sprintf(dupArg[0], "%s%s%s", dupArg[0], holdPid, dupArg2[0]);
        }
        free(holdPid);
        arguments[x] = dupArg[0];
      }

      // Handling '~/' variable expansion.
      if (homeExpansion == 1){
        char* holdHome = calloc(128, sizeof(char));
        
        sprintf(holdHome, "%s", home);
        sprintf(dupArg[0], "%s%s", holdHome, newArg2);
        arguments[x] = dupArg[0];
      }

      // Handling '$?' variable expansion.
      if (exitExpansion == 1){
        char* holdExit = calloc(16, sizeof(char));

        sprintf(holdExit, "%d", statusExit);
        if (noPost == 1){
          sprintf(dupArg[0], "%s%s", dupArg[0], holdExit);
        } else {
          sprintf(dupArg[0], "%s%s%s", dupArg[0], holdExit, dupArg2[0]);
        }
        arguments[x] = dupArg[0];
      }

      // Handling background status expansion.
      if (backgroundExpansion == 1){
        sprintf(dupArg[0], "%s", dupArg[0]);
        arguments[x] = dupArg[0];
      }
      
      // Add original argument if no expansion occurs.
      if (noExpansion == 0) arguments[x] = arg;
      
      // Getting next token.
      x++;
      arg = strtok(NULL, ifs);
      
      // End of tokenization loop. ----------------------
    }


    // If no args were tokenized, somehow, back to input.
    if (x == 0) continue;
    
    
    // Checking for "&" as last argument to enter Background Mode.
    if (x > 1 && strcmp(arguments[x-1], "&") == 0){
      backgroundMode = 1;
      
      // Removes "&" from argument list, and decrement args.
      arguments[x-1] = NULL;
      x--;
    }
    

    // Getting file names for input/output redirection.
    if (inFlag == 1 || outFlag == 1){
    
      // Case where only '<' is given.
      if (inFlag == 1 && outFlag == 0){
      
        // Remove from arguments if '<' precedes last word.
        if (strcmp(arguments[x-2], "<") == 0){
          strcpy(fileStdin, arguments[x-1]);
          arguments[x-2] = NULL;
          arguments[x-1] = NULL;
          x--;
          x--;
        } else {
          inFlag = 0;
        }
      }
      
      // Case where only '>' is given.
      else if (inFlag == 0 && outFlag == 1){
          if (strcmp(arguments[x-2], ">") == 0){
            strcpy(fileStdout, arguments[x-1]);
            arguments[x-2] = NULL;
            arguments[x-1] = NULL;
            x--;
            x--;
          } else {
            outFlag = 0;
          }
          
      // Case where both '<' and '>' are given, in any order.
      } else {
      
            // Case if stdout occurs before stdin.
            if (strcmp(arguments[x-2], "<") == 0 && strcmp(arguments[x-4], ">") == 0){
              strcpy(fileStdin, arguments[x-1]);
              strcpy(fileStdout, arguments[x-3]);
              arguments[x-4] = NULL;
              arguments[x-3] = NULL;
              arguments[x-2] = NULL;
              arguments[x-1] = NULL;
              x-=4;

            // Case if stdin occurs before stdout.
            } else if (strcmp(arguments[x-2], ">") == 0 && strcmp(arguments[x-4], "<") == 0){
              strcpy(fileStdout, arguments[x-1]);
              strcpy(fileStdin, arguments[x-3]);
              arguments[x-4] = NULL;
              arguments[x-3] = NULL;
              arguments[x-2] = NULL;
              arguments[x-1] = NULL;
              x-=4;
              
            // Else, '<' and '>' are in invalid positions for redirection, so they are treated as args.  
            } else {
              inFlag = 0;
              outFlag = 0;
            }
          }        
        }
    // -------------- End of tokenization -----------------------------


    // ---- Built-in 'exit' command. ----------
    if (strcmp(arguments[0], "exit") == 0) {
      builtIn = 1;

        // More than 2 arguments.
        if (x > 2) {
          fprintf(stderr, "Error: too many arguments for 'exit'(1 at most).\n");
          continue;
          
         // Exactly 2 arguments.
        } else if (x == 2) {
          exitStatus = strtol(arguments[1], &end, 10);
          if (end == exitNum) {
          
            // Argument is not an integer.
            fprintf(stderr, "Error: argument is not a supported exit value.\n");
            continue;
            
          // Else, exit with status as provided by 2nd argument.
          } else {
            fprintf(stderr, "\nexit\n");
            statusExit = (int)exitStatus;
            exit(statusExit);
          }
          
        // Exactly 1 argument, exit with last exit status.
        } else if (x == 1){
          fprintf(stderr, "\nexit\n");
          exit(statusExit);
        }
      }


    // ----- Built-in 'cd' command. -------------
    if (strcmp(arguments[0], "cd") == 0){
      builtIn = 1;
      
      // If 2 or more arguments, print error.
      if (x > 2) {
        fprintf(stderr, "Error: too many arguments passed to 'cd'.\n");
        continue;
        
      // If 'cd' has 1 argument, change directory to path from argument.
      } else if (x == 2) {     
        dir = chdir(arguments[1]);
        if (dir == -1){
          fprintf(stderr, "Error: could not change directory to provided path.\n");
          continue;
        }
        
      // If 'cd' has no arguments, change directory to default path, 'home'.
      } else if (x == 1) {        
        chdir(home);
      }
    }
   
   
    // Start from beginning if a 'built-in' command was executed.
    if (builtIn == 1) {
      continue;
    }


    // Executing loop.
    for (;;){
      
      // Forking a child process.
      sPid = fork();

      switch(sPid){
        case -1: {
                   // Child process failed to fork.
                   fprintf(stderr, "Process failed to fork.\n");
                   exit(1);
                   break;
                 }
        case 0: {
                  if (x == 1){
                    // One argument to execute.
                    if (execlp(arguments[0], arguments[0], NULL) == -1){
                      fprintf(stderr, "Child process failed to execute, command not found.\n");
                      exit(1);
                    }
                    
                  } else {
                    // Handling input redirection.
                    if (inFlag == 1){
                      int openFile = open(fileStdin, O_RDONLY);
                      if (openFile == -1){
                        fprintf(stderr, "Error: file could not be opened, or does not exist.\n");
                        exit(1);
                      }
                      int res = dup2(openFile, 0);
                      if (res == -1){
                        fprintf(stderr, "Error: could not redirect stdin to file.\n");
                        exit(1);
                      }
                      fcntl(openFile, F_SETFD, FD_CLOEXEC);
                    }

                    // Handling output redirection.
                    if (outFlag == 1){
                      int openFile = open(fileStdout, O_WRONLY | O_CREAT, 0777);
                      if (openFile == -1){
                        fprintf(stderr, "Error: file could not be opened.\n");
                        exit(1);
                      }
                      int res = dup2(openFile, 1);
                      if (res == -1){
                        fprintf(stderr, "Error: could not redirect stdout to file.\n");
                        exit(1);
                      }
                      fcntl(openFile, F_SETFD, FD_CLOEXEC);
                    }

                    // Multiple arguments to execute.
                  if (execvp(arguments[0], arguments) == -1){
                      fprintf(stderr, "Child process failed to execute, command not be found.\n");
                      exit(1);
                    }
                    
                  }
                  break;
                }
        default:{
                  pid_t wpid = waitpid(sPid, &statusExit, 0);
                }
      }
      break;
    }
  }
}

void freeArgs(char* arguments[]){
  for(int i = 0; i < 512; ++i){
    arguments[i] = NULL;
  }
}

void freeDups(char* arguments[]){
  for (int i = 0; i < 256; i++){
    arguments[i] = NULL;
  }
}

/*void freeCommands(char commands[]){
  for (int i = 0; i < 2048; ++i){
    commands[i] = NULL;
  }
}*/

void printArguments(char* arguments[], int argSize){
  for(int i = 0; i < argSize; ++i){
    if(arguments[i] == NULL) break;
    printf("%d: %s\n", i, arguments[i]);
    fflush(stdout);
  }
}


void removeLine(char command[]){
  int x = 0;
  for(;;){
    if (command[x] == '\n') {
      command[x] = '\0';
      break;
    } else {
      x++;
      // Might cause issue later.
      if (x > 100) break;
    }
  }
}
