#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
#include <cstdio>
#include "utils.hpp"
using namespace std;

// System call includes
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

extern char **environ;
// Assign this to background jobs.
unsigned int nextJobId = 0;

// Executes a list of commands, piping each to the next successively.
int executeCommandList( const vector<Command> & commandList );

int main( int argc, char **argv, char **envp )
{
	initZombieReaping();

	// Can handle both STDIN redirected at startup of Quash, or taking user input
	// until the user enters an EOF character (ctrl + D).
	while( cin.good() )
	{
		// Display a prompt if not running a script from stdin.
		if( isatty( STDIN_FILENO ) )
		{
			cout << "[" << get_current_dir_name() << "]$ ";
		}

		vector<Command> commandList = getInput( cin );
		// If it's an empty list of commands, try and get another.
		if( commandList.size() == 0 )
		{
			continue;
		}
		// If the command list contains a shell builtin, we'll be careful.
		else if( containsShellBuiltin( commandList ) )
		{
			// If it's a single shell builtin, just run it.
			if( commandList.size() == 1 )
			{
				Command command = commandList[0];
				if( (string)command.argv[0] == "exit" || (string)command.argv[0] == "quit" )
				{
					exit( 0 );
				}
				if( (string)command.argv[0] == "set" )
				{
					set( command.argv );
				}
				else if( (string)command.argv[0] == "cd" )
				{
					cd( command.argv );
				}
				else if( (string)command.argv[0] == "jobs" )
				{
					jobs();
				}
				else if( (string)command.argv[0] == "kill" )
				{
					kill( command.argv );
				}
				else if( (string)command.argv[0] == "help")
				{
					help();
				}
			}
			// If it's a list of commands with any shell builtin, don't run it, because
			// we haven't defined what STDIN and STDOUT should do for shell builtins.
			// Why you'd want to pipe them, I don't know, but Quash won't let you.
			else
			{
				cerr << "Piping with shell builtins is undefined. Type help for list of shell builtins." << endl;
				continue;
			}
		}
		// If it's a list of one or more non-shell-builtin commands, run it!
		else
		{
			executeCommandList( commandList );
		}
	}

	return 0;
}

// Executes a list of commands, piping each to the next successively.
// This function is kind of a bear, but it's the workhorse of Quash.
int executeCommandList( const vector<Command> & commandList )
{
	// Save STDIN and STDOUT to restore them in case piping messes something up.
	int stdin_saved = dup( STDIN_FILENO );
	int stdout_saved = dup( STDOUT_FILENO );

	// We'll later save the pid of the first child created. This will be the
	// pid reported if this set of commands is intended to run in the background.
	pid_t firstChildPid = 0;

	pid_t pid;
	int status;
	// Start with input from STDIN.
	int inputfd = STDIN_FILENO;
	int pipefd[2];
	// If there is more than one command in the list, i.e. a series of commands
	// intended to be piped successively, this for loop handles that.
	for( unsigned int i = 0; i < commandList.size() - 1; i++ )
	{
		// Create the pipe.
		pipe( pipefd );

		pid = fork();
		if( pid < 0 )
		{
			cerr << "Fork failed." << endl;
		}
		else if( pid == 0 )
		{
			// If the input for this child isn't STDIN.
			if( inputfd != STDIN_FILENO )
			{
				// Rename STDIN for this child to inputfd, which should be
				// the read end of the pipe of the last command.
				dup2( inputfd, STDIN_FILENO );
				close( inputfd );
			}
			// Rename STDOUT to the current pipe's write end.
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
			// Save the pid of the first child for background job reporting.
			if( commandList[i].executeInBackground )
			{
				if( i == 0 )
				{
					firstChildPid = pid;
				}
			}
			if( !commandList[i].executeInBackground )
			{
				// Parent waits for child to finish.
				wait( &status );
			}
		}

		// Cleanup, and set inputfd to the read end of this command, to be
		// attached to STDIN of the next process.
		close( pipefd[1] );
		inputfd = pipefd[0];
	}

	// If inputfd isn't STDIN, it must be the read end of the pipe of the last
	// process, so attach it to STDIN.
	if( inputfd != STDIN_FILENO )
	{
		dup2( inputfd, STDIN_FILENO );
	}

	// The final command in the pipe. (Or if there was only one command in
	// this command list, this would technically be the first and only command,
	// since the above for loop would have been skipped.
	Command command = commandList[commandList.size() - 1];
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
		// If there was only one command in the command list, then this child
		// must be the first child created, so store its pid.
		if( commandList.size() == 1 )
		{
			firstChildPid = pid;
		}
		// If this command list was intended to run in the background, save it
		// into the list of background jobs.
		if( command.executeInBackground )
		{
			Job job;
			// Put together all of the separate commands that were piped together.
			job.command += commandList[0].rawString;
			for( unsigned int i = 1; i < commandList.size(); i++ )
			{
				job.command += ( " | " + commandList[i].rawString );
			}
			job.jobId = nextJobId++;
			job.pid = firstChildPid;	// Set the pid of the job to the pid of the first child.
			backgroundJobs.push_back( job );
			cout << "[" << job.jobId << "] " << job.pid << " running in background." << endl;
		}
		else
		{
			// Parent waits for child to finish.
			wait( &status );
		}
	}

	// Clean up the pipes.
	close( pipefd[0] );
	close( pipefd[1] );
	// Make sure STDIN and STDOUT go back to normal in case the pipes screwed something up.
	dup2( stdin_saved, STDIN_FILENO );
	dup2( stdout_saved, STDOUT_FILENO );

	return EXIT_SUCCESS;
}

