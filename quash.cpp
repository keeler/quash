#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
using namespace std;

// System call includes
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

extern char **environ;

// Runs the command if it's a shell built-in (like cd, set, etc.). Returns
// false if the command isn't a built-in.
bool runShellBuiltIn( char **args );
// Split into a list of substrings based on the delimiter.
// Like split in Perl or Python.
vector<string> split( const string & str, char delimiter );
// Removes the characters in the string whitespace from the beginning and end of the string.
string trim( const string & str, const string & whitespace );
// Creates an argument list that can be passed to execve().
char **createArgv( const string & commandAndArgs );
// Frees the argument list created by createArgv().
void freeArgv( char **argv );
//take command methods
void take_command(string command, int status);

int main( int argc, char **argv, char **envp )
{
	while( 1 )
	{
		string command;
		int status = 0;

		// Display a prompt.
		cout << "[" << get_current_dir_name() << "]" << " $ ";
		// Get a line from the user.
		getline( cin, command );
    //send to function
    take_command(command, status);
  }
  return 0;
}

void take_command(string command, int status)
{
		// If the user just hit enter (and nothing else), do nothing.
		if( command.length() == 0 )
		{
      //no longer in loop, can return here
			//continue;
      return;
		}

		// Turn the command into something we can pass to execve().
		char **args = createArgv( command );

		// Run shell built-ins here. Returns false if the
		// command wasn't a built-in like cd or set.
		if( runShellBuiltIn( args ) )
		{
			freeArgv( args );
			//continue;
      return;
		}

		// Fork a child to run the user's command
		pid_t pid = fork();
		if( pid < 0 )
		{
			cerr << "Fork failed." << endl;
		}
		else if( pid == 0 )
		{
			// If it's an absolute path, just give that to exec().
			if( args[0][0] == '/' )
			{
				if( access( args[0], F_OK ) != 0 )
				{
					cerr << "File \"" << args[0] << "\" does not exist." << endl;
					exit( 0 );
				}
				if( access( args[0], X_OK ) != 0 )
				{
					cerr << "File \"" << args[0] << "\" is not an executable." << endl;
					exit( 0 );
				}
				if( ( execve( args[0], args, environ ) < 0 ) )
				{
					cerr << "Error executing command \"" << args[0] << "\", ERROR #" << errno << "." << endl;
					//return EXIT_FAILURE;
				  //dont know how to handle that
          return;
        }
			}
			// If the user puts a "./" in front of the command, just
			// look in the present working directory.
			else if( args[0][0] == '.' && args[0][1] == '/' )
			{
				if( access( &args[0][2], F_OK ) != 0 )
				{
					cerr << "File \"" << &args[0][2] << "\" does not exist in current directory." << endl;
					exit( 0 );
				}
				if( access( args[0], X_OK ) != 0 )
				{
					cerr << "File \"" << &args[0][2] << "\" is not an executable." << endl;
					exit( 0 );
				}
				if( ( execve( &args[0][2], args, environ ) < 0 ) )
				{
					cerr << "Error executing command \"" << args[0] << "\", ERROR #" << errno << "." << endl;
          //return EXIT_FAILURE;
				  //dont know how to handle that
          return;
        }
			}
			// Try to find the executable in one of the paths given by the
			// PATH environment variables.
			else
			{
				vector<string> PATH = split( getenv( "PATH" ), ':' );
				for( unsigned int i = 0; i < PATH.size(); i++ )
				{
					string cmd = PATH[i] + "/" + args[0];
					if( access( cmd.c_str(), F_OK ) != 0 )
					{
						continue;
					}
					if( access( cmd.c_str(), X_OK ) != 0 )
					{
						cerr << "File found at \"" << cmd << "\" using PATH is not an executable." << endl;
						exit( 0 );
					}
					if( ( execve( ( PATH[i] + "/" + args[0] ).c_str(), args, environ ) < 0 ) )
					{
						cerr << "Error executing command \"" << command << "\", ERROR #" << errno << "." << endl;
						//return EXIT_FAILURE;
					  //dont know how to handle that
            return;
          }
				}
			}

			cerr << "File \"" << args[0] << "\" does not exist in any of the directories in PATH." << endl;
			// Exit here avoids nested child processes if an exec failed.
			exit( 0 );
		}
		else
		{
			// Parent waits for child to finish.
			wait( &status );
		}

		// Clean up to avoid memory leaks.
		freeArgv( args );
}


bool runShellBuiltIn( char **args )
{
	// If the user types exit or quit, break out of the infinite loop.
	if( (string)args[0] == "exit" || (string)args[0] == "quit" )
	{
		freeArgv( args );
		exit( 0 );
	}
	//set for HOME and PATH
	//environment variables are only changed during execution of quash
	if( (string)args[0] == "set" )
	{
		if( strncmp( args[1], "HOME=", 5 ) == 0 )
		{
			if( setenv( "HOME", &args[1][5], 1 ) == -1 )
			{
				cerr << "Error setting HOME,  ERROR #" << errno << "." << endl;	
			}
		}
		else if( strncmp( args[1], "PATH=", 5 ) == 0 )
		{
			if( setenv( "PATH", &args[1][5], 1 ) == -1 )
			{ 
				cerr << "Error setting PATH,  ERROR #" << errno << "." << endl;	
			}
		}
	}
	else if( (string)args[0] == "cd" )
	{
		if( args[1] )	// If there is a directory to change to
		{
			// If it's an absolute path.
			if( args[1][0] == '/' )
			{
				if( chdir( args[1] ) != 0 )
				{
					cerr << "Couldn't change directory to \"" << args[1] << "\", ERROR #" << errno << "." << endl;
				}
			}
			else
			{
				string pwd = get_current_dir_name();
				if( chdir( ( pwd + "/" + args[1] ).c_str() ) != 0 )
				{
					cerr << "Couldn't change directory to \"" << pwd + "/" + args[1] << "\", ERROR #" << errno << "." << endl;
				}
			}
		}
		else
		{
			if( chdir( getenv( "HOME" ) ) != 0 )
			{
				cerr << "Couldn't change directory to \"" << getenv( "HOME" ) << "\", ERROR #" << errno << "." << endl;
			}
		}
	}
	else if( (string)args[0] == "kill" )
	{
		if( args[1] ) //if theres a process id
		{
			if( kill( atoi( args[1] ), SIGKILL ) != 0 )
			{
				cerr << "Couldn't kill process " << args[1] << ", ERROR #" << errno << "." << endl;
			}
		}
	}
	//Load commands from a file
	else if ( (string)args[0] == "<")
	{
		if( args[1] ) //if theres a file to load from
		{
      ifstream infile;
      infile.open(args[1]);
      if(infile)//if file exists
      {
        string s;
        while(!infile.eof()){
          getline(infile,s); 
          take_command(s,0);
        }
      }
      infile.close();
		}		

	}	
	//Help menu for Quash
	//Lists off all the functions Quash can do
	//will be used for report
  else if( (string)args[0] == "help")
  {
      cout<<"\n\nQuash 1.0 \n";
      cout<<"Written by Keeler Russell and Jeff Cailteux\n";
      cout<<"For EECS 678\n\n";
      cout<<"Help Menu\n\n";

			cout<<"cd <directory>\n";
			cout<<"exit\n";
			cout<<"jobs\n";			
			cout<<"kill <process id>\n";
			cout<<"quit\n";
			cout<<"set <environment variable>\n";
			cout<<"<  <text file>\n";

			cout<<endl;
  }
	else
	{
		return false;
	}

	return true;
}

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

string trim( const string & str, const string & whitespace )
{
	int strBegin = str.find_first_not_of( whitespace );
	int strEnd = str.find_last_not_of( whitespace );
	int strRange = strEnd - strBegin + 1;

	return str.substr( strBegin, strRange );
}

char **createArgv( const string & commandAndArgs )
{
	vector<string> arguments = split( commandAndArgs, ' ' );
	unsigned int numArgs = arguments.size();

	char **argv = new char*[numArgs + 1];	// Need + 1 to add extra NULL for execve()
	for( unsigned int i = 0; i < numArgs; i++ )
	{
		argv[i] = new char[arguments[i].length() + 1];	// Need room for NULL terminator
		strncpy( argv[i], arguments[i].c_str(), arguments[i].length() );
		argv[i][arguments[i].length()] = '\0';
	}
	argv[numArgs] = NULL;

	return argv;
}

void freeArgv( char **argv )
{
	for( unsigned int i = 0; argv[i]; i++ )
	{
		delete [] argv[i];
	}

	delete [] argv;
}

