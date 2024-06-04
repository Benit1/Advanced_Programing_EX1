# Advanced Programing EX1
## submitting : Beni Tibi 208434290 Or Ben Ami 318417763
### In this EX we create a custom shell implementation in C that supports various features including command execution, history management, variable handling, and basic flow control constructs. This project is a part of an assignment to enhance the basic shell capabilities.
#### -- The program is written in the C programming language using Clion and To compile the program, we use the GCC (GNU Compiler Collection) compiler.

## Features

 **Execute Commands**: Supports executing commands with arguments.
 
 **Redirect Output**: Redirect output to files using `>`, `>>`, and `2>`.
 
 **Background Execution**: Run commands in the background using `&`.
 
 **Change Prompt**: Customize the shell prompt.
 
 **Echo Command**: Display arguments and variable values.
 
 **Print Status**: Print the status of the last executed command using `echo $?`.
 
 **Change Directory**: Change the current working directory using `cd`.
 
 **Repeat Last Command**: Repeat the last command using `!!`.
 
 **Exit Shell**: Exit the shell using `quit`.
 
 **Signal Handling**: Handle `Control-C` with a custom message.
 
 **Piped Commands**: Support for piped command execution.
 
 **Variables**: Set and use variables within the shell.
 
 **Read Command**: Read user input and store it in a variable.
 
 **History Navigation**: Navigate through command history using arrow keys.
 
 **Flow Control**: Support for basic flow control constructs like `if/else`.


 

## examples:
 ![image](https://github.com/Benit1/Advanced_Programing_EX1/assets/110784868/9b7448c9-e067-45a2-91d7-f534d034d8d0)
in this picture you can see for example the command $? which return 1 because the "if" conditaion fails and its prints "b"
so the output will be 1.

to run the program we need to do this steps:
first compile with the "make all" command.
after that we need to go inside our shell by doing ./myshell.
inside the shell we created we can do our features.
to exit the shell we need to write "quit".




 
 
