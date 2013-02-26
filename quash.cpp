#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
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

// Execute command given by argv array. Handles shell builtins like cd or set.
int executeCommand( char **argv );

int main( int argc, char **argv, char **envp )
{
	initZombieReaping();

	while( 1 )
	{
		string command;

		// Display a prompt.
		cout << "[" << get_current_dir_name() << "]$ ";
		char **args = getCommand( cin );
		if( args == NULL )
		{
			continue;
		}

		executeCommand( args );

		// Clean up to avoid memory leaks.
		freeArgv( args );
	}

	return 0;
}

int executeCommand( char **argv )
{
	if( (string)argv[0] == "exit" || (string)argv[0] == "quit" )
	{
		quit( argv );
	}
	if( (string)argv[0] == "set" )
	{
		set( argv );
		return EXIT_SUCCESS;
	}
	else if( (string)argv[0] == "cd" )
	{
		cd( argv );
		return EXIT_SUCCESS;
	}
	else if( (string)argv[0] == "kill" )
	{
		kill( argv );
		return EXIT_SUCCESS;
	}
	else if( (string)argv[0] == "help")
	{
		help( argv );
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
		// If it's an absolute path, just give that to exec().
		if( argv[0][0] == '/' )
		{
			// Returns 0 if file exists and is an executable.
			if( executableExists( argv[0] ) != 0 )
			{
				exit( 0 );
			}
			if( ( execve( argv[0], argv, environ ) < 0 ) )
			{
				cerr << "Error executing command \"" << argv[0] << "\", ERROR #" << errno << "." << endl;
				return EXIT_FAILURE;
			}
		}
		// If the user puts a "./" in front of the command, just
		// look in the present working directory.
		else if( argv[0][0] == '.' && argv[0][1] == '/' )
		{
			if( executableExists( &argv[0][2] ) != 0 )
			{
				exit( 0 );
			}
			if( ( execve( &argv[0][2], argv, environ ) < 0 ) )
			{
				cerr << "Error executing command \"" << argv[0] << "\", ERROR #" << errno << "." << endl;
				return EXIT_FAILURE;
			}
		}
		// Try to find the executable in one of the paths given by the
		// PATH environment variables.
		else
		{
			vector<string> PATH = split( getenv( "PATH" ), ':' );
			for( unsigned int i = 0; i < PATH.size(); i++ )
			{
				string cmd = PATH[i] + "/" + argv[0];
				if( access( cmd.c_str(), F_OK ) != 0 )
				{
					continue;
				}
				if( access( cmd.c_str(), X_OK ) != 0 )
				{
					cerr << "File found at \"" << cmd << "\" using PATH is not an executable." << endl;
					exit( 0 );
				}
				if( ( execve( ( PATH[i] + "/" + argv[0] ).c_str(), argv, environ ) < 0 ) )
				{
					cerr << "Error executing command \"" << cmd << "\", ERROR #" << errno << "." << endl;
					return EXIT_FAILURE;
				}
			}
		}
		
		cerr << "File \"" << argv[0] << "\" does not exist in any of the directories in PATH." << endl;
		// Exit here avoids nested child processes if an exec failed.
		exit( 0 );
	}
	else
	{
		// Parent waits for child to finish.
		wait( &status );
	}

	return EXIT_SUCCESS;
}

