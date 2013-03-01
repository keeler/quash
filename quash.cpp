#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdio>
#include "utils.hpp"
#include "builtins.hpp"
using namespace std;

// System call includes
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

extern char **environ;
unsigned int nextJobId = 0;

// Execute command given by argv array. Handles shell builtins like cd or set.
int executeCommand( const Command & command );

int main( int argc, char **argv, char **envp )
{
	initZombieReaping();

	while( 1 )
	{
		// Display a prompt.
		cout << "[" << get_current_dir_name() << "]$ ";
		Command command = getCommand( cin );
		if( command.argv == NULL )
		{
			continue;
		}

		executeCommand( command );
	}

	return 0;
}

int executeCommand( const Command & command )
{
	if( (string)command.argv[0] == "exit" || (string)command.argv[0] == "quit" )
	{
		exit( 0 );
	}
	if( (string)command.argv[0] == "set" )
	{
		set( command.argv );
		return EXIT_SUCCESS;
	}
	else if( (string)command.argv[0] == "cd" )
	{
		cd( command.argv );
		return EXIT_SUCCESS;
	}
	else if( (string)command.argv[0] == "jobs" )
	{
		jobs();
		return EXIT_SUCCESS;
	}
	else if( (string)command.argv[0] == "kill" )
	{
		kill( command.argv );
		return EXIT_SUCCESS;
	}
	else if( (string)command.argv[0] == "help")
	{
		help( command.argv );
		return EXIT_SUCCESS;
	}

	int status;
	// Fork a child to run the user's command
	pid_t pid = fork();
	if( pid < 0 )
	{
		cerr << "Fork failed." << endl;
	}
	else if( pid == 0 )
	{
		if( command.executeInBackground )
		{
			// Put child in new process group.
			setpgid( 0, 0 );
			// As with bash, disable input from terminal, but leave output
			// output enabled. UNLESS there is an input file.
			if( command.inputFilename.length() == 0 )
			{
				close( STDIN_FILENO );
			}
		}

		if( command.inputFilename.length() > 0 )
		{
			FILE *ifile = fopen( command.inputFilename.c_str(), "r" );
			if( ifile == NULL )
			{
				cerr << "Couldn't open \"" << command.inputFilename << "\"." << endl;
				exit( EXIT_FAILURE );
			}

			dup2( fileno( ifile ), STDIN_FILENO );
			fclose( ifile );
		}
		if( command.outputFilename.length() > 0 )
		{
			FILE *ofile = fopen( command.outputFilename.c_str(), "w" );
			if( ofile == NULL )
			{
				cerr << "Couldn't open \"" << command.outputFilename << "\"." << endl;
				exit( EXIT_FAILURE );
			}

			dup2( fileno( ofile ), STDOUT_FILENO );
			fclose( ofile );
		}

		exit( execute( command.argv ) );
	}
	else
	{
		if( command.executeInBackground )
		{
			Command job = command;
			job.jobId = nextJobId++;
			job.pid = pid;
			backgroundJobs.push_back( job );
			cout << "[" << job.jobId << "] " << job.pid << " running in backround." << endl;
		}
		else
		{
			// Parent waits for child to finish.
			wait( &status );
		}
	}

	return EXIT_SUCCESS;
}

