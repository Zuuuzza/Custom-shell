# Custom Shell Project

This project is a custom shell implementation. It supports command-line input as well as file input. The following commands have been implemented:

- `exit`: Terminates the shell process.
- `lcd [path]`: Changes the current directory to the specified path.
- `lkill [ -signal_number ] pid`: Sends the signal_number to the process/group pid.
- `lls`: Lists the names of the files in the current directory (similar to `ls -1` without any options).

The shell supports redirection of input and output and command piping. It also supports running programs in the background and signal handling.
