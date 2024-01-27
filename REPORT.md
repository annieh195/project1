# SSHELL: Simple Shell
## Summary
This program, `sshell`, can run UNIX commands as well as builtin functions `pwd`
, `cd`, `exit`, and `sls`. It also has output redirection for append and 
truncate commands along with piping capabilities. 

## Implementation
The implementation of this program follows six consecutive phases:
1. Running simple commands by using `fork`+`exec`+`wait`.
2. Handling command lines containing programs and its arguments.
3. Implementing builtin functions `pwd`, `cd`, and `exit`
4. Implementing output redirection follows by two steps
    1. Parsing of the output redirection from the command line.
    2. Manipulating the stdout file descriptor prior to running the specified
    program.
5. Implementing pipeline commands
6. Extra features (append and sls)

### Running simple commands
Commands are parsed into an array and `fork` is called. The child will be the 
only one running the commands by using `execvp`. The parent will `wait` for the 
child to finish before continuing to ask for user input.

### Handling command lines
The command string is split and tokenized using `strtok`. The tokens are stored
into an array to be passed into `execvp`.

### Implmenting builtin functions
Commands are compared with keywords like `pwd`, `cd`, and `exit`. If the
commands match, then the functions from C's standard library are used to carry
out the command. `pwd` will use the function `getcwd` and print the path. `cd`
will use `chdir` on its argument. `exit` will let the user know they are exiting
the shell and change a local variable to prompt an exit out of the `while` loop.

### Implementing output redirection
The command parser searches for the metacharacter `>` with `strchr`. If it's
found, it separates the output file from the command with and stores it. After
`fork` is executed, the child will check for a valid output file and reroute the
output of the command to the output file. To reroute, it uses `open` to get the
file descriptor of the specified output file. `dup2` changes the output to file
descriptor. `close` is finally used to close the file descriptor. When the 
output is rerouted, the child will run the command with `execvp`.

### Implementing pipeline commands
The command parser tokenizes the inputted command based on the metacharacter 
`|`. If it's valid, it will keep track of the number of pipes needed. Each 
individual command will also then be tokenized into its arguments and stored
into a two-dimensional array. The array dedicates each command to a single row, 
which will be passed into `execvp` instead of the whole array. After parsing, an
array whose size is determined by the number of pipes needed is created. There
is only one array, but different pipes are created by iteratively sectioning off
two cells and calling the `pipe` function on those cells. The parent will then
`fork` multiple times, determined by a `for` loop that depends on the number of
commands inputted. The children will use `dup2` to reroute the output to the
write part of the pipe if the child is not about to run the last command. It
will also reroute the input to the read part of the pipe if the child is not
about to run the first command. The child will then `close` all of the pipes and
2@run the command with `execvp`. The parent will also `close` all of the pipes
then `wait`s for the child to exit.

### Extra features
#### Append
The command parser uses the same technique in output redirection to search for 
the metacharacter `>>` to determine if the user wants to append to an output
file. If it's found, it uses a local variable to keep track of the intention to
append. When `fork` is called, the child will check the local variable and
call a different `open` with the `O_APPEND` parameter. `dup2` will redirect the 
output to the file descriptor using that `open`. The child will `close` the file
descriptor and run the command with `execvp`. 
#### Implement builtin function sls
Command is compared with keyword `sls`. If the commands match, then the 
functions from C's standard library such as `opendir`, `readdir`, and `stat` 
with the `S_ISREG` are used to open the current directory, iterate over its 
entries, and check if each is a regular file. It then prints the name and size 
of each regular file found.

## License
This work is distributed under the [GNU All-Permissive
License](https://spdx.org/licenses/FSFAP.html).