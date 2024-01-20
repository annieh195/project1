# SSHELL: Simple Shell
## Summary
This program, `sshell`, can 

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
6.
### Running simple commands

### Handling command lines

## License
This work is distributed under the [GNU All-Permissive
License](https://spdx.org/licenses/FSFAP.html).
