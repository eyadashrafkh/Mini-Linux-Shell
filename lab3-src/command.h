
#ifndef command_h
#define command_h

#include <bits/stdc++.h>

/*	Prototypes	*/
void handler_SIGINT(int);
void handleSIGCHLD(int);
int changeCurrentDirectory(void);
void add_dir_to_path(char *);
void removeNewline(char *, int);
void openLogFile();
void closeLogFile();

// Command Data Structure
struct SimpleCommand
{
	// Available space for arguments currently preallocated
	int _numberOfAvailableArguments;

	// Number of arguments
	int _numberOfArguments;
	char **_arguments;

	SimpleCommand();
	//void wildcard(char* s);
	void insertArgument(char *argument);
};

struct Command
{
	int _numberOfAvailableSimpleCommands;
	int _numberOfSimpleCommands;
	SimpleCommand **_simpleCommands;
	char *_outFile;
	char *_inputFile;
	char *_errFile;
	int _background;
	int _append;
	int _wildcard;

	void prompt();
	void print();
	void execute();
	void clear();

	Command();
	void insertSimpleCommand(SimpleCommand *simpleCommand);

	static Command _currentCommand;
	static SimpleCommand *_currentSimpleCommand;
};

#endif
