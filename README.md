# Shell
An implementation of the shell program written in C language.

Important points about the shell.c file-

Compile and run the shell.c file. When ./a.out is executed the shell starts and is invoked from the home directory of the shell and is shown by ~.

Built-in commands cd, pwd and echo have been implemented using separate functions.

Other system commands can be executed as in a regular shell.

Semi-colon separated commands can also be executed.

Commands used - gethostname, getlogin_r, getcwd, fork, waitpid, chdir, execvp.

strtok has been used to separate enter command into arguments.

Redirection of commands is possible.

Piping with redirection has been implemented.

A process can be sent to the background with the '&' sign

A process can be brought to the foreground by typing 'fg' before the process id.

Other built-in commands - jobs, kjob, fg, overkill and quit.
