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
		quit( command.argv );
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
		// If it's an absolute path, just give that to exec().
		if( command.argv[0][0] == '/' )
		{
			// Returns 0 if file exists and is an executable.
			if( executableExists( command.argv[0] ) != 0 )
			{
				exit( 0 );
			}
			if( ( execve( command.argv[0], command.argv, environ ) < 0 ) )
			{
				cerr << "Error executing command \"" << command.argv[0] << "\", ERROR #" << errno << "." << endl;
				return EXIT_FAILURE;
			}
		}
		// If the user puts a "./" in front of the command, just
		// look in the present working directory.
		else if( command.argv[0][0] == '.' && command.argv[0][1] == '/' )
		{
			if( executableExists( &command.argv[0][2] ) != 0 )
			{
				exit( 0 );
			}
			if( ( execve( &command.argv[0][2], command.argv, environ ) < 0 ) )
			{
				cerr << "Error executing command \"" << command.argv[0] << "\", ERROR #" << errno << "." << endl;
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
				string cmd = PATH[i] + "/" + command.argv[0];
				if( access( cmd.c_str(), F_OK ) != 0 )
				{
					continue;
				}
				if( access( cmd.c_str(), X_OK ) != 0 )
				{
					cerr << "File found at \"" << cmd << "\" using PATH is not an executable." << endl;
					exit( 0 );
				}
				if( ( execve( ( PATH[i] + "/" + command.argv[0] ).c_str(), command.argv, environ ) < 0 ) )
				{
					cerr << "Error executing command \"" << cmd << "\", ERROR #" << errno << "." << endl;
					return EXIT_FAILURE;
				}
			}
		}
		
		cerr << "File \"" << command.argv[0] << "\" does not exist in any of the directories in PATH." << endl;
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

