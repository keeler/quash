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
// Executes a list of commands, piping each to the next successively.
int executeCommandList( const vector<Command> & commandList );

int main( int argc, char **argv, char **envp )
{
	initZombieReaping();

	while( 1 )
	{
		// Display a prompt.
		cout << "[" << get_current_dir_name() << "]$ ";
		cin.clear();
		vector<Command> commandList = getInput( cin );
		if( commandList.size() == 0 )
		{
			continue;
		}
		else
		{
			executeCommandList( commandList );
		}
	}

	return 0;
}

int executeCommandList( const vector<Command> & commandList )
{
	Command command = commandList[0];
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

	int stdin_saved = dup( STDIN_FILENO );
	int stdout_saved = dup( STDOUT_FILENO );

	pid_t pid;
	int status;
	int inputfd = STDIN_FILENO;
	int pipefd[2];
	for( unsigned int i = 0; i < commandList.size() - 1; i++ )
	{
		pipe( pipefd );

		pid = fork();
		if( pid < 0 )
		{
			cerr << "Fork failed." << endl;
		}
		else if( pid == 0 )
		{
			if( inputfd != 0 )
			{
				dup2( inputfd, STDIN_FILENO );
				close( inputfd );
			}
			dup2( pipefd[1], STDOUT_FILENO );
			close( pipefd[1] );

			if( commandList[i].executeInBackground )
			{
				// Put child in new process group.
				setpgid( 0, 0 );
			}

			if( redirectStdIn( commandList[i].inputFilename ) != 0 )
			{
				exit( EXIT_FAILURE );
			}
			if( redirectStdOut( commandList[i].outputFilename ) != 0 )
			{
				exit( EXIT_FAILURE );
			}

			exit( execute( commandList[i].argv ) );
		}
		else
		{
			if( commandList[i].executeInBackground )
			{
				Job job;
				job.command = commandList[i];
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

		close( pipefd[1] );
		inputfd = pipefd[0];
	}

	if( inputfd != 0 )
	{
		dup2( inputfd, STDIN_FILENO );
	}

	// The final command in the pipe.
	command = commandList[commandList.size() - 1];
	pid = fork();
	if( pid < 0 )
	{
		cerr << "Fork failed." << endl;
		exit( 0 );
	}
	else if( pid == 0 )
	{
		if( command.executeInBackground )
		{
			// Put child in new process group.
			setpgid( 0, 0 );
		}

		if( redirectStdIn( command.inputFilename ) != 0 )
		{
			exit( EXIT_FAILURE );
		}
		if( redirectStdOut( command.outputFilename ) != 0 )
		{
			exit( EXIT_FAILURE );
		}
	
		exit( execute( command.argv ) );
	}
	else
	{
		if( command.executeInBackground )
		{
			Job job;
			job.command = command;
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

	close( pipefd[0] );
	close( pipefd[1] );
	dup2( stdin_saved, STDIN_FILENO );
	dup2( stdout_saved, STDOUT_FILENO );

	return EXIT_SUCCESS;
}

