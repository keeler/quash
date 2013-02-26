#include "utils.hpp"
#include "Command.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
using namespace std;

// Helper for access() system call. Returns -1 if
// file not found, 1 if not an executable, and 0
// if the file was found and is an executable.
int executableExists( const std::string & filename )
{
	if( access( filename.c_str(), F_OK ) != 0 )
	{
		cerr << "File \"" << filename << "\" does not exist." << endl;
		return -1;
	}
	if( access( filename.c_str(), X_OK ) != 0 )
	{
		cerr << "File \"" << filename << "\" is not an executable." << endl;
		return 1;
	}

	return 0;
}

// SIGCHLD signal handler to reap zombie processes.
void sigchldHandler( int signal )
{
	while( waitpid( -1, NULL, WNOHANG ) > 0 );
}

// Set up the above SIGCHLD handler so it will go into action.
void initZombieReaping()
{
	struct sigaction sa;

	sa.sa_handler = sigchldHandler;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = SA_RESTART;

	if( sigaction( SIGCHLD, &sa, NULL ) == -1 )
	{
		cerr << "Failure in sigaction() call." << endl;
		exit( 1 );
	}
}

// Creates an argument list that can be passed to execve()
char **createArgv( const string & commandAndArgs )
{
	vector<string> arguments = split( commandAndArgs, ' ' );
	unsigned int argc = arguments.size();

	char **argv = new char*[argc + 1];	// Need +1 for extra NULL for execve()
	for( unsigned int i = 0; i < argc; i++ )
	{
		argv[i] = new char[arguments[i].length() + 1];
		strncpy( argv[i], arguments[i].c_str(), arguments[i].length() );
		argv[i][arguments[i].length()] = '\0';
	}
	argv[argc] = NULL;

	return argv;
}

// Gets a command from the given input stream and turns it into an argv array.
Command getCommand( std::istream & is )
{
	Command cmd;
	string command;
	getline( is, command );

	vector<string> tokens = split( command, ' ' );
	for( unsigned int i = 0; i < tokens.size(); i++ )
	{
		if( tokens[i] == "<" )
		{
			if( i + 1 < tokens.size() )
			{
				cmd.inputFilename = tokens[++i];
			}
			else
			{
				cerr << "Error parsing input command: \"<\" must be followed by an input filename." << endl;
				return cmd;
			}
		}
		else if( tokens[i] == ">" )
		{
			if( i + 1 < tokens.size() )
			{
				cmd.outputFilename = tokens[++i];
			}
			else
			{
				cerr << "Error parsing input command: \">\" must be followed by an output filename." << endl;
				return cmd;
			}
		}
		else if( tokens[i] == "&" )
		{
			cmd.executeInBackground = true;
		}
		else
		{
			cmd.command += ( " " + tokens[i] );
		}
	}

	cmd.command = trim( cmd.command );
	if( cmd.command.length() > 0 )
	{
		cmd.argv = createArgv( cmd.command );
	}

	return cmd;
}

// Split str into tokens based on delimiter, like split in Perl or Python
vector<string> split( const string & str, char delimiter )
{
	stringstream ss( str );
	string token;
	vector<string> tokens;

	while( getline( ss, token, delimiter ) )
	{
		if( !token.empty() )
		{
			tokens.push_back( token );
		}
	}

	return tokens;
}

// Trim whitespace from beginning and end of string
string trim( const string & str, const string & whitespace )
{
	int begin = str.find_first_not_of( whitespace );
	int end = str.find_last_not_of( whitespace );
	int range = end - begin + 1;

	return str.substr( begin, range );
}

