#include "utils.hpp"
#include "Command.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
using namespace std;

vector<Job> backgroundJobs;

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
	pid_t pid;
	while( ( pid = waitpid( -1, NULL, WNOHANG ) ) > 0 )
	{
		// Remove background job that just finished from the list.
		// Note that backgroundJobs is an extern global.
		for( unsigned int i = 0; i < backgroundJobs.size(); i++ )
		{
			if( backgroundJobs[i].pid == pid )
			{
				Job job = backgroundJobs[i];
				cout << endl << "[" << job.jobId << "] " << job.pid << " finished " << job.command << endl;
				backgroundJobs.erase( backgroundJobs.begin() + i );
				break;
			}
		}
	}
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

// Run execve() on the given command, searching through $PATH if needed. 
int execute( char **argv )
{
	// If it's an absolute path, just give that to exec().
	if( argv[0][0] == '/' )
	{
		// Returns 0 if the file exists and is an executable.
		if( executableExists( argv[0] ) != 0 )
		{
			return EXIT_FAILURE;
		}
		if( execve( argv[0], argv, environ ) < 0 )
		{
			cerr << "Error executing \"" << argv[0] << "\", errno = " << errno << "." << endl;
			return EXIT_FAILURE;
		}
	}
	// If the user puts a "./" in front of the command, just look in the
	// current working directory for the executable.
	else if( strncmp( argv[0], "./", 2 ) == 0 )
	{
		// Returns 0 if the file exists and is an executable.
		if( executableExists( &argv[0][2] ) != 0 )
		{
			return EXIT_FAILURE;
		}
		if( execve( &argv[0][2], argv, environ ) < 0 )
		{
			cerr << "Error executing \"" << &argv[0][2] << "\", errno = " << errno << "." << endl;
			return EXIT_FAILURE;
		}
	}
	// Try to find the executable in one of the paths given by the PATH
	// environment variable.
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
				return EXIT_FAILURE;
			}
			if( execve( cmd.c_str(), argv, environ ) < 0 )
			{
				cerr << "Error executing \"" << cmd << "\", errno = " << errno << "." << endl;
				return EXIT_FAILURE;
			}
		}
	}

	cerr << "Executable named \"" << argv[0] << "\" does not exist in any of the directories in PATH." << endl;
	return EXIT_FAILURE;
}

int redirectStdIn( const string & filename )
{
	if( filename.length() > 0 )
	{
		FILE *ifile = fopen( filename.c_str(), "r" );
		if( ifile == NULL )
		{
			cerr << "Couldn't open \"" << filename << "\"." << endl;
			return -1;
		}

		dup2( fileno( ifile ), STDIN_FILENO );
		fclose( ifile );
	}

	return 0;
}

int redirectStdOut( const string & filename )
{	
	if( filename.length() > 0 )
	{
		FILE *ofile = fopen( filename.c_str(), "w" );
		if( ofile == NULL )
		{
			cerr << "Couldn't open \"" << filename << "\"." << endl;
			return -1;
		}

		dup2( fileno( ofile ), STDOUT_FILENO );
		fclose( ofile );
	}

	return 0;
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
vector<Command> getInput( std::istream & is )
{
	string rawInput;
	getline( is, rawInput );
	vector<Command> result;
	vector<Command> emptyVector;	// For error returns;

	if( rawInput.length() == 0 )
	{
		return result;
	}

	bool runInBackground = false;
	if( rawInput.find( "&" ) != string::npos )
	{
		runInBackground = true;
		rawInput.erase( rawInput.find( "&" ), 1 );
	}

	// Put spaces around file redirect so tokenizer doesn't get confused.
	// "|" and "&" are not an issue as the code is.
	rawInput = replaceAll( rawInput, "<", " < " );
	rawInput = replaceAll( rawInput, ">", " > " );

	vector<string> subCommands = split( rawInput, '|' );
	for( unsigned int i = 0; i < subCommands.size(); i++ )
	{
		Command cmd;
		cmd.rawString = trim( subCommands[i] );

		vector<string> tokens = split( subCommands[i], ' ' );
		string parseString;
		for( unsigned int j = 0; j < tokens.size(); j++ )
		{
			if( tokens[j] == "<" )
			{
				if( j + 1 < tokens.size() )
				{
					cmd.inputFilename = tokens[++j];
				}
				else
				{
					cerr << "Error parsing input command: \"<\" must be followed by an input filename." << endl;
					return emptyVector;
				}
			}
			else if( tokens[j] == ">" )
			{
				if( j + 1 < tokens.size() )
				{
					cmd.outputFilename = tokens[++j];
				}
				else
				{
					cerr << "Error parsing input command: \">\" must be followed by an output filename." << endl;
					return emptyVector;
				}
			}
			else
			{
				parseString += ( " " + tokens[j] );
			}
		}

		parseString = trim( parseString );
		cmd.argv = createArgv( parseString );

		cmd.executeInBackground = runInBackground;

		result.push_back( cmd );
	}

	return result;
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

std::string replaceAll( const string & str, const string & before, const string & after )
{
	string result;
	string::const_iterator curr = str.begin();
	string::const_iterator end = str.end();
	string::const_iterator next = search( curr, end, before.begin(), before.end() );

	while( next != end )
	{
		result.append( curr, next );
		result.append( after );
		curr = next + before.size();
		next = search( curr, end, before.begin(), before.end() );
	}

	result.append( curr, next );
	return result;
}

void cd( char **argv )
{
	// If there's a directory given to change to
	if( argv[1] )
	{
		// If it's an absolute path
		if( argv[1][0] == '/' )
		{
			if( chdir( argv[1] ) != 0 )
			{
				cerr << "Couldn't change directory to \"" << argv[1] << "\", ERROR #" << errno << "." << endl;
			}
		}
		else
		{
			string pwd = get_current_dir_name();
			if( chdir( ( pwd + "/" + argv[1] ).c_str() ) != 0 )
			{
				cerr << "Couldn't change directory to \"" << pwd + "/" + argv[1] << "\", ERROR #" << errno << "." << endl;
			}
		}
	}
	// Otherwise, it's just the string 'cd', so change to HOME
	else
	{
		if( chdir( getenv( "HOME" ) ) != 0 )
		{
			cerr << "Couldn't change directory to \"" << getenv( "HOME" ) << "\", ERROR #" << errno << "." << endl;
		}
	}
}

void set( char **argv )
{
	// If no environment variable specified, tell user what quash
	// expects and do nothing.
	if( !argv[1] )
	{
		cerr << "Need to specifiy PATH=... or HOME=..." << endl;
		return;
	}

	// Set HOME with system call.
	if( strncmp( argv[1], "HOME=", 5 ) == 0 )
	{
		if( setenv( "HOME", &argv[1][5], 1 ) == -1 )
		{
			cerr << "Error setting HOME, ERROR #" << errno << "." << endl;
		}
	}
	// Set PATH with system call.
	else if( strncmp( argv[1], "PATH=", 5 ) == 0 )
	{
		if( setenv( "PATH", &argv[1][5], 1 ) == -1 )
		{
			cerr << "Error setting PATH, ERROR #" << errno << "." << endl;
		}
	}
}

void jobs()
{
	if( backgroundJobs.size() > 0 )
	{
		cout << "[JOBID]\tPID\tCOMMAND" << endl;
	}
	for( unsigned int i = 0; i < backgroundJobs.size(); i++ )
	{
		cout << "[" << backgroundJobs[i].jobId << "]\t" << backgroundJobs[i].pid << "\t" << backgroundJobs[i].command << endl;
	}
}

void kill( char **argv )
{
	// If no PID specified, tell user what quash
	// expects and do nothing.
	if( !argv[1] )
	{
		cerr << "Must specify process ID of process to kill." << endl;
		return;
	}

	if( kill( atoi( argv[1] ), SIGINT ) != 0 )
	{
		cerr << "Couldn't kill process " << argv[1] << ", ERROR #" << errno << "." << endl;
	}
}

void help()
{
	cout << "################################################" << endl;
	cout << "#                  Quash 1.0                   #" << endl;
	cout << "#                Quite A Shell!                #" << endl;
	cout << "#            (KU EECS 678 Project 1)           #" << endl;
	cout << "#                                              #" << endl;
	cout << "#                   Authors:                   #" << endl;
	cout << "#                Keeler Russell                #" << endl;
	cout << "#                Jeff Cailteux                 #" << endl;
	cout << "################################################" << endl << endl;

	cout << "USAGE" << endl;
	cout << "-----" << endl;
	cout << "Interactive Mode: simply run the executable with no arguments." << endl;
	cout << "Script Mode: run the executable, and pipe your script to it with" << endl;
	cout << "             stdin. E.g. quash < scriptname.sh." << endl << endl;

	cout << "SHELL BUILTINS" << endl;
	cout << "--------------" << endl;
	cout << "1) cd <directory>" << endl;
	cout << "    - If no argument given, changes to $HOME." << endl;
	cout << "2) exit" << endl;
	cout << "3) quit" << endl;
	cout << "4) jobs" << endl;
	cout << "    - Prints list of jobs currently running in the background." << endl;
	cout << "5) kill <process id>" << endl;
	cout << "    - Sends SIGKILL signal to process with the given process ID." << endl;
	cout << "6) set <evironment variable>" << endl;
	cout << "    - Only two environment variables recognized: HOME and PATH." << endl;
	cout << "    - E.g. set HOME=/home/johndoe" << endl;
	cout << "    - E.g. set PATH=/bin:/usr/bin" << endl;
	cout << "    - Directories for PATH must be separated by colons." << endl;
}

bool containsShellBuiltin( const std::vector<Command> & commandList )
{
	for( unsigned int i = 0; i < commandList.size(); i++ )
	{
		Command cmd = commandList[i];
		if( strcmp( cmd.argv[0], "exit" ) == 0 ||
			strcmp( cmd.argv[0], "quit" ) == 0 ||
			strcmp( cmd.argv[0], "cd" ) == 0 ||
			strcmp( cmd.argv[0], "jobs" ) == 0 ||
			strcmp( cmd.argv[0], "kill" ) == 0 ||
			strcmp( cmd.argv[0], "set" ) == 0 ||
			strcmp( cmd.argv[0], "help" ) == 0 )
		{
			return true;
		}
	}

	return false;
}

