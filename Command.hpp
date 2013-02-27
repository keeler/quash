#ifndef _COMMAND_HPP_
#define _COMMAND_HPP_

#include <string>
#include <cstring>
#include <sys/types.h>

struct Command
{
	std::string 	command;				// Command sent to Quash.
	std::string		inputFilename;			// Input file (for redirected stdin).
	std::string		outputFilename;			// Ouput file (for redirected stdout).
	char			**argv;					// ARGV string to pass to execve().
	bool			executeInBackground;	// Whether to run in background.
	unsigned int	jobId;					// Job ID assigned when job put into background.
	pid_t			pid;					// Process ID of job.

	// Default constructor
	Command() :
		command( "" ),
		inputFilename( "" ),
		outputFilename( "" ),
		argv( NULL ),
		executeInBackground( false ),
		jobId( 0 ),
		pid( 0 )
	{
	}

	// Copy constructor
	Command( const Command & that ) :
		command( that.command ),
		inputFilename( that.inputFilename ),
		outputFilename( that.outputFilename ),
		executeInBackground( that.executeInBackground ),
		jobId( that.jobId ),
		pid( that.pid )
	{
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
		if( argv != NULL )
		{
			for( unsigned int i = 0; argv[i]; i++ )
			{
				delete [] argv[i];
			}
			delete [] argv;
		}
	}

	Command & operator=( const Command & that )
	{
		if( this != &that )
		{
			command = that.command;
			inputFilename = that.inputFilename;
			outputFilename = that.outputFilename;
			executeInBackground = that.executeInBackground;
			jobId = that.jobId;
			pid = that.pid;

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

