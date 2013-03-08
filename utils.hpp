#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <vector>
#include <string>
#include "Command.hpp"

// A job that will go in the list of background jobs.
struct Job
{
	std::string		command;	// E.g. du /home/username | grep eecs | sort --numeric --reverse | head
	unsigned int	jobId;		// Unique, assigned by quash.
	pid_t			pid;		// Pid of first child process forked when running the job.
};

// Make the list of running background jobs an extern global so it can be used
// in multiple source files.
extern std::vector<Job> backgroundJobs;

///////////////////////////////
// Wrappers for system calls //
///////////////////////////////

// Helper for multiple sequential access() system calls.
int executableExists( const std::string & filename );
// SIGCHLD signal handler to reap zombie processes.
void sigchldHandler( int signal );
// Set up the above SIGCHLD handler so it will go into action.
void initZombieReaping();
// Run execve() for the given command, searching through $PATH if needed.
int execute( char **argv );
// Redirects STDIN to read from given filename, if it exists.
// Only redirects for non-empty string.
int redirectStdIn( const std::string & filename );
// Redirects STDOUT to output to given filename. File will be
// created if it does not exist. Only redirects for non-empty
// string.
int redirectStdOut( const std::string & filename );

///////////////////////////
// Argument manipulation //
///////////////////////////

// Creates an argument list that can be passed to execve()
char **createArgv( const std::string & commandAndArgs );
// Gets a command from the given input stream and turns
// it into an argv array.
std::vector<Command> getInput( std::istream & is );
// Runs the command if there is at least one shell builtin function (e.g. "cd"
// or "set", type help for all) among the series of piped commands. Returns
// true if it ran, false if it didn't.
bool runIfShellBuiltin( const std::vector<Command> & commandList );

////////////////////////////
// Text-munging utilities //
////////////////////////////

// Split str into tokens based on delimiter, like split in Perl or Python
std::vector<std::string> split( const std::string & str, char delimiter );
// Trim whitespace from beginning and end of string
std::string trim( const std::string & str, const std::string & whitespace = " \t\n" );
// Replace all occurrences of before with after in the string str.
std::string replaceAll( const std::string & str, const std::string & before, const std::string & after  );

////////////////////
// Shell builtins //
////////////////////

void cd( char **argv );
void set( char **argv );
void jobs();
void kill( char **argv );
void help();

bool containsShellBuiltin( const std::vector<Command> & commandList );

#endif

