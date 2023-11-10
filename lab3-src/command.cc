
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctime>

#include "command.h"


SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_append = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );

	int inp, outp, err;

	//if command has error file
	if (_errFile) {
		err = open(_errFile, O_WRONLY | O_CREAT, 0777);
		dup2(err, 2);
	}
	//if command has input file
	if (_inputFile) {
		inp = open(_inputFile, O_RDONLY, 0777);
	}
	//if command has output file
	if (_outFile) {
		if (!_append)
			outp = open(_outFile, O_WRONLY | O_CREAT, 0777);
		else
			outp = open(_outFile, O_WRONLY | O_APPEND, 0777);
	}

	// Create file descriptor 
	int outfd[_numberOfSimpleCommands][2];

	// loop through all nested commands
	for (int i = 0; i < _numberOfSimpleCommands; i++){
		
		// Create new pipe 
		// Conceptually, a pipe is a connection between two processes, 
		// such that the standard output from one process becomes the standard input of the other process.
		// so if a process writes to outfd[1] process be can read from fdpipe[0] 
		pipe(outfd[i]);
		
		// Changing directory
		if (strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0)
		{
			printf("\n");
			// Call function to change directory
			if (changeCurrentDirectory() == -1)
				// 033 is for red color of the message
				printf("\033[31mError occurred. Make sure the directory you entered is valid\033[0m\n");
			continue;
		}

		// Print contents of Command data structure
		print();

		// Add execution here
		// For every simple command fork a new process
		// Setup i/o redirection
		// and call exec
		
		// first command
		if (i == 0){
			// Redirect input 
			if (_inputFile)
			{
				dup2(inp, 0);
				close(inp);
			}
			else
				dup2(defaultin, 0);
		}
		else {
			dup2(outfd[i - 1][0], 0);
			close(outfd[i - 1][0]);
		}
		if (i == _numberOfSimpleCommands -1){
			// Redirect output
			if (_outFile)
				dup2(outp, 1);
			else 
				dup2(defaultout, 1);
		}
		else {
			dup2(outfd[i][1], 1);
			close(outfd[i][1]);
		}

		// Create new process 
		int pid = fork();
		if (pid == -1) {
			printf("%s: fork\n", _simpleCommands[i]->_arguments[0]);
			perror("");
			exit(2);
		}

		if (pid == 0) {
			// Child
			// You can use execvp() instead if the arguments are stored in an array
			execvp(_simpleCommands[i]->_arguments[0], &_simpleCommands[i]->_arguments[0]);
		}
		else{
			// Parent
			signal(SIGCHLD, handleSIGCHLD);
			// Restore input and output
			dup2(defaultin, 0);
			dup2(defaultout, 1);
			// Wait for child condition
			if (!_background)
				waitpid(pid, 0, 0);
		}
	}

	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void
Command::prompt()
{
	signal(SIGINT, handler_SIGINT);
	printf("myshell>");
	// Print current directory
	for (int i = 0; i < next_dir; i++)
		printf("%s>", path_to_current_directory[i]);
	printf(" ");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

/* Main Functions*/

// Global variables
FILE *fp;
int next_dir = 0;
char *home_dir = getenv("HOME");
char *path_to_current_directory[128];

// Child log file

char LOG_FILE_NAME[] = "/child-log.txt";
void openLogFile() {
	char path_to_log[64];
	strcpy(path_to_log, getenv("HOME"));
	strcat(path_to_log, LOG_FILE_NAME);
	fp = fopen(path_to_log, "a");
}

void closeLogFile() {
	fclose(fp);
}

void handleSIGCHLD(int sig_num)
{
	int status;
	wait(&status);
	openLogFile();
	flockfile(fp);
	time_t TIMER = time(NULL);
	tm *ptm = localtime((&TIMER));
	char currentTime[32];
	strcpy(currentTime, asctime(ptm));
	removeNewline(currentTime, 32);
	fprintf(fp, "%s: Child Terminated\n", currentTime);
	funlockfile(fp);
	fclose(fp);
	signal(SIGCHLD, handleSIGCHLD);
}

void removeNewline(char *str, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (str[i] == '\n')
		{
			str[i] = '\0';
			return;
		}
	}
}


// SIGINT handler to catch ctrl-c

void handler_SIGINT(int sig_num)
{
	signal(SIGINT, handler_SIGINT);
	Command::_currentCommand.clear();
	printf("\r\033[0J"); // Erase myshell> ^C
	Command::_currentCommand.prompt();
	fflush(stdout);
}


// Change directory command

int changeCurrentDirectory()
{
	// Return 0 if successful, -1 if not
	int returnValue;

	// Get the path after the command 'cd' 
	// if none is given, go to home directory
	char *path = Command::_currentSimpleCommand->_arguments[1];

	if (path)
		returnValue = chdir(path);
	else
		returnValue = chdir(home_dir);

	if (returnValue == 0 || !path)
		add_dir_to_path(path);

	Command::_currentCommand.clear();
	return returnValue;
}

void add_dir_to_path(char *dir)
{
	if (dir == NULL)
		next_dir = 0;
	else if (strcmp(dir, "..") == 0 || strcmp(dir, ".") == 0)
	{
		if (next_dir > 0)
			next_dir--;
	}
	else
		path_to_current_directory[next_dir++] = dir;
}


/* End of Main Functions*/

int 
main()
{
	chdir(home_dir);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

