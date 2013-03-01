#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <iostream>
#include <vector>
#include <string>
#include "Command.hpp"

// Make the list of running background jobs an extern global so it can be used
// in multiple source files.
extern std::vector<Command> backgroundJobs;

// Wrappers for system calls
// Helper for multiple sequential access() system calls.
int executableExists( const std::string & filename );
// SIGCHLD signal handler to reap zombie processes.
void sigchldHandler( int signal );
// Set up the above SIGCHLD handler so it will go into action.
void initZombieReaping();
// Run execve() for the given command, searching through $PATH if needed.
int execute( char **argv );

// Argument manipulation
// Creates an argument list that can be passed to execve()
char **createArgv( const std::string & commandAndArgs );
// Gets a command from the given input stream and turns
// it into an argv array.
Command getCommand( std::istream & is );

// Text-munging utilities
// Split str into tokens based on delimiter, like split in Perl or Python
std::vector<std::string> split( const std::string & str, char delimiter );
// Trim whitespace from beginning and end of string
std::string trim( const std::string & str, const std::string & whitespace = " \t\n" );

#endif

