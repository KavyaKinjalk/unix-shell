/*
Kavya Kinjalka
kavk
119054697
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <err.h>

#include "command.h"
#include "executor.h"
#include <sysexits.h>

static int execute_aux(struct tree *t, int p_input_fd, int p_output_fd);
int execute_none(struct tree *t, int p_input_fd, int p_output_fd);
int execute_and(struct tree *t, int p_input_fd, int p_output_fd);
int execute_pipe(struct tree *t, int p_input_fd, int p_output_fd);
int execute_subshell(struct tree *t, int p_input_fd, int p_output_fd);


int execute(struct tree *t) {
   return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
}

static int execute_aux(struct tree *t, int p_input_fd, int p_output_fd) {
   if (t == NULL){
      return 0;
   }

   switch (t->conjunction) {
      case NONE:
         return execute_none(t, p_input_fd, p_output_fd);
      case AND:
         return execute_and(t, p_input_fd, p_output_fd);
      case PIPE:
         return execute_pipe(t, p_input_fd, p_output_fd);
      case SUBSHELL:
         return execute_subshell(t, p_input_fd, p_output_fd);
      default:
         fprintf(stderr, "Some error?.\n");
         return -1;
   }
}

int execute_none(struct tree *t, int p_input_fd, int p_output_fd) {
   pid_t pid;
   /* Check for special built-in commands like 'exit' or 'cd'. */
   if (strcmp(t->argv[0], "exit") == 0) {
      exit(0);
   } else if (strcmp(t->argv[0], "cd") == 0) {
      if (t->argv[1] == NULL) {
         chdir(getenv("HOME"));
      } else {
         chdir(t->argv[1]);
      }
      return 0;
   }

   /* Fork a new process for executing the command. */
   pid = fork();
   if (pid == 0) {  
      /* Handle input redirection if specified. */
      if (t->input) {
         int in_fd = open(t->input, O_RDONLY);
         if (in_fd == -1) {
            err(EX_OSERR, "Failed to read file");
         }
         if (dup2(in_fd, STDIN_FILENO) == -1) {
            err(EX_OSERR, "Failed to duplicate file descriptor");
         }
         close(in_fd);
      } else if (p_input_fd) {
         if (dup2(p_input_fd, STDIN_FILENO) == -1) {
            err(EX_OSERR, "Failed to duplicate file descriptor");
         }
      }

      /* Handle output redirection if specified. */
      if (t->output) {
         int out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
         if (out_fd == -1) {
            err(EX_OSERR, "Failed to open output file");
         }
         dup2(out_fd, STDOUT_FILENO);
         close(out_fd);
      } else if (p_output_fd != STDOUT_FILENO) {
         dup2(p_output_fd, STDOUT_FILENO);
      }

      execvp(t->argv[0], t->argv);
      err(EX_OSERR, "ececvp failed");
   } else if (pid > 0) { 
      /* Wait for the child to complete and capture its exit status. */
      int status;
      waitpid(pid, &status, 0);
      return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
   } else {
      err(EX_OSERR, "fork failed");
   }
}

int execute_and(struct tree *t, int p_input_fd, int p_output_fd) {
    /* Execute the left child first. */
    int left_status = execute_aux(t->left, p_input_fd, p_output_fd);
    /* Execute the right child if the left child succeeded. */
    if (left_status == 0) return execute_aux(t->right, p_input_fd, p_output_fd);
    return left_status;  /* Return the status of the left child if it failed */
}

int execute_pipe(struct tree *t, int p_input_fd, int p_output_fd) {
   pid_t left_pid, right_pid;
   /* Create a pipe to connect the output of one command to the input of another. */
   int pipefd[2];
   if (t->left->output || t->right->input) {
      printf("Ambiguous output redirect.\n");
      return -1;
   }
   if (pipe(pipefd) == -1) {
      err(EX_OSERR, "pipe error");
   }

   /* Fork the process for the left command. */
   left_pid = fork();
   if (left_pid == 0) {  /* Left child */
      dup2(pipefd[1], STDOUT_FILENO);  /* Redirect stdout to the pipe's write end */
      close(pipefd[0]);  
      close(pipefd[1]);  
      execute_aux(t->left, p_input_fd, STDOUT_FILENO);
      exit(EXIT_FAILURE);
   }

   /* Fork the process for the right command. */
   right_pid = fork();
   
   if (right_pid == 0) {  
      dup2(pipefd[0], STDIN_FILENO);  /* Redirect stdout to the pipe's read end */
      close(pipefd[1]);
      close(pipefd[0]); 
      execute_aux(t->right, STDIN_FILENO, p_output_fd);
      exit(EXIT_FAILURE);
   }

   close(pipefd[0]);
   close(pipefd[1]);
   waitpid(left_pid, NULL, 0);
   waitpid(right_pid, NULL, 0);
   return 0;
}

int execute_subshell(struct tree *t, int p_input_fd, int p_output_fd) {
   int status;
   pid_t pid = fork();
  
   if (pid == 0) { 
      /* Handle input redirection */
      if (t->input) {
         int in_fd = open(t->input, O_RDONLY);
         if (in_fd == -1) {
               err(EX_OSERR,"Cannot open file");
         }
         dup2(in_fd, STDIN_FILENO);
         close(in_fd);
      } else if (p_input_fd != STDIN_FILENO) {
         dup2(p_input_fd, STDIN_FILENO);
      }

      /* Handle output redirection */
      if (t->output) {
         int out_fd = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
         if (out_fd == -1) {
               err(EX_OSERR,"Cannot open file");
         }
         dup2(out_fd, STDOUT_FILENO);
         close(out_fd);
      } else if (p_output_fd != STDOUT_FILENO) {
         dup2(p_output_fd, STDOUT_FILENO);
      }

      /* Execute the subtree inside the subshell */
      status = execute_aux(t->left, STDIN_FILENO, STDOUT_FILENO);
      exit(status);  /* Return the status from the executed subtree */
   } else if (pid > 0) {
      int executionStatus;
      waitpid(pid, &executionStatus, 0);
      return WIFEXITED(executionStatus) ? WEXITSTATUS(executionStatus) : -1;
   } else {
      err(EX_OSERR, "fork error");
   }
}
