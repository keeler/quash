#include "builtins.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <signal.h>
using namespace std;

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

void quit( char **argv )
{
	exit( 0 );
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

	if( kill( atoi( argv[1] ), SIGKILL ) != 0 )
	{
		cerr << "Couldn't kill process " << argv[1] << ", ERROR #" << errno << "." << endl;
	}
}

void help( char **argv )
{
	cout << "\n\nQuash 1.0" << endl;
	cout << "Written by Keeler Russell and Jeff Cailteux" << endl;
	cout << "For EECS 678" << endl << endl;

	cout << "Help Menu" << endl << endl;

	cout << "cd <directory>" << endl;
	cout << "exit" << endl;
	cout << "jobs" << endl;
	cout << "kill <process id>" << endl;
	cout << "quit" << endl;
	cout << "set <evironment variable>" << endl << endl;
}

