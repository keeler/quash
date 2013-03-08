#ifndef _COMMAND_HPP_
#define _COMMAND_HPP_

#include <string>
#include <cstring>
#include <sys/types.h>

struct Command
{
	std::string 	rawString;				// Command sent to Quash. (e.g. "ls -al | grep e > out" is two commands, "ls -al" and "grep e > out"
	std::string		inputFilename;			// Input file (for redirected stdin). Empty string if redirect not speficied.
	std::string		outputFilename;			// Ouput file (for redirected stdout). Empty string if redirect not specified.
	char			**argv;					// ARGV string to pass to execve().
	bool			executeInBackground;	// Whether to run in background.

	// Default constructor
	Command() :
		rawString( "" ),
		inputFilename( "" ),
		outputFilename( "" ),
		argv( NULL ),
		executeInBackground( false )
	{
	}

	// Copy constructor
	Command( const Command & that ) :
		rawString( that.rawString ),
		inputFilename( that.inputFilename ),
		outputFilename( that.outputFilename ),
		executeInBackground( that.executeInBackground )
	{
		// Copy the argv array if it's not NULL.
		if( that.argv != NULL )
		{
			unsigned int argc = 0;
			for( argc = 0; that.argv[argc]; argc++ );

			argv = new char*[argc + 1];
			for( unsigned int i = 0; i < argc; i++ )
			{
				argv[i] = new char[strlen( that.argv[i] ) + 1];
				strncpy( argv[i], that.argv[i], strlen( that.argv[i] ) );
				argv[i][strlen( that.argv[i] )] = '\0';
			}
			argv[argc] = NULL;
		}
		else
		{
			argv = NULL;
		}
	}

	~Command()
	{
		// Only need to explicitly destroy this, all other member variables
		// have their own destructors.
		if( argv != NULL )
		{
			for( unsigned int i = 0; argv[i]; i++ )
			{
				delete [] argv[i];
			}
			delete [] argv;
		}
	}

	// Overloaded assignment operator. Makes passing Commands around possible
	// without memory errors.
	Command & operator=( const Command & that )
	{
		if( this != &that )
		{
			rawString = that.rawString;
			inputFilename = that.inputFilename;
			outputFilename = that.outputFilename;
			executeInBackground = that.executeInBackground;

			if( that.argv != NULL )
			{
				unsigned int argc = 0;
				for( argc = 0; that.argv[argc]; argc++ );

				argv = new char*[argc + 1];
				for( unsigned int i = 0; i < argc; i++ )
				{
					argv[i] = new char[strlen( that.argv[i] ) + 1];
					strncpy( argv[i], that.argv[i], strlen( that.argv[i] ) );
					argv[i][strlen( that.argv[i] )] = '\0';
				}
				argv[argc] = NULL;
			}
			else
			{
				argv = NULL;
			}
		}

		return *this;
	}
};

#endif

