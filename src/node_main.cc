// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "node.h"

#ifdef _WIN32
int wmain(int argc, wchar_t *wargv[]) {
  // Convert argv to to UTF8
  char** argv = new char*[argc];
  for (int i = 0; i < argc; i++) {
    // Compute the size of the required buffer
    DWORD size = WideCharToMultiByte(CP_UTF8,
                                     0,
                                     wargv[i],
                                     -1,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL);
    if (size == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
    // Do the actual conversion
    argv[i] = new char[size];
    DWORD result = WideCharToMultiByte(CP_UTF8,
                                       0,
                                       wargv[i],
                                       -1,
                                       argv[i],
                                       size,
                                       NULL,
                                       NULL);
    if (result == 0) {
      // This should never happen.
      fprintf(stderr, "Could not convert arguments to utf8.");
      exit(1);
    }
  }
  // Now that conversion is done, we can finally start.
  return node::Start(argc, argv);
}
#else

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

// UNIX
//returns new argc
int removeArgument(int argc, char** argv, int argIdx)
{
	if((argIdx < 0) || (argIdx >= argc))	return(argc);
	
	for(int i = argIdx; i < (argc - 1); i++)
		argv[i] = argv[i + 1];
	
	return(argc - 1);
}

//returns index of desired argument or -1 if not found
int indexOfArgument(int argc, char** argv, char const* desiredArg)
{
	for(int i = 0; i < argc; i++)
		if(strcmp(argv[i], desiredArg) == 0)
			return(i);
	
	return(-1);	//not found
}

void exitWithError(char const* err, char const* str = 0)
{
	fprintf(stderr, err, str);
	exit(-1);
}

int main(int argc, char *argv[])
{
	//implement daemonize options:
	
	//duplicate argv so we can eat options at this level
	char** args = new char*[argc];
	for(int i = 0; i < argc; i++)
		args[i] = argv[i];
	
	bool daemonize = false;
	//scan argv for these options and remove them from the list
	int argIdx;
	if(((argIdx = indexOfArgument(argc, args, "-d")) != -1) || ((argIdx = indexOfArgument(argc, args, "--daemonize")) != -1))
	{
		daemonize = true;
		argc = removeArgument(argc, args, argIdx);
	}
	
	if(daemonize)
	{
		//hook up fds 0, 1 and 2 (stdin, stdout and stderr, respectively) to files provided by the -0, -1 and -2 arguments
		
		FILE* pidfile = 0;
		if(((argIdx = indexOfArgument(argc, args, "-f")) != -1) || ((argIdx = indexOfArgument(argc, args, "--pidfile")) != -1))
		{
			if((argIdx + 1) < argc)
			{
				if((pidfile = fopen(args[argIdx + 1], "w")) == 0)
					exitWithError("Error: Could not open pidfile, exiting.\n");
			}
			else	exitWithError("Error: No pidfile specified, exiting.\n");
			
			argc = removeArgument(argc, args, argIdx + 1);
			argc = removeArgument(argc, args, argIdx);
		}
		
		//find arguments like "-0" or "--stdin"
		int newStdin = -1;
		if(((argIdx = indexOfArgument(argc, args, "-0")) != -1) || ((argIdx = indexOfArgument(argc, args, "--stdin")) != -1))
		{
			//try to open the file
			if((argIdx + 1) < argc)
			{
				if((newStdin = open(args[argIdx + 1], O_RDONLY)) == -1)
					exitWithError("Error: Could not open stdin file '%s', exiting.\n", args[argIdx + 1]);
			}
			else	exitWithError("Error: No stdin file specified, exiting.\n");
			
			//remove the argument and the one following it.
			argc = removeArgument(argc, args, argIdx + 1);
			argc = removeArgument(argc, args, argIdx);
		}
		
		int newStdout = -1;
		if(((argIdx = indexOfArgument(argc, args, "-1")) != -1) || ((argIdx = indexOfArgument(argc, args, "--stdout")) != -1))
		{
			//try to open the file
			if((argIdx + 1) < argc)
			{
				if((newStdout = open(args[argIdx + 1], O_WRONLY | O_CREAT | O_APPEND, 0640)) == -1)
					exitWithError("Error: Could not open stdout file '%s', exiting.\n", args[argIdx + 1]);
			}
			else	exitWithError("Error: No stdout file specified, exiting.\n");
			
			//remove the argument and the one following it.
			argc = removeArgument(argc, args, argIdx + 1);
			argc = removeArgument(argc, args, argIdx);
		}
		
		int newStderr = -1;
		if(((argIdx = indexOfArgument(argc, args, "-2")) != -1) || ((argIdx = indexOfArgument(argc, args, "--stderr")) != -1))
		{
			//try to open the file
			if((argIdx + 1) < argc)
			{
				if((newStderr = open(args[argIdx + 1], O_WRONLY | O_CREAT | O_APPEND, 0640)) == -1)
					exitWithError("Error: Could not open stderr file '%s', exiting.\n", args[argIdx + 1]);
			}
			else	exitWithError("Error: No stderr file specified, exiting.\n");
			
			//remove the argument and the one following it.
			argc = removeArgument(argc, args, argIdx + 1);
			argc = removeArgument(argc, args, argIdx);
		}
		
		//handle defaults for fds 0, 1 and 2
		if(newStdin == -1)	//if not specified, use /dev/null for stdin
			newStdin = open("/dev/null", O_RDONLY);
		
		if(newStdout == -1)	//if not specified, use /dev/null for stdout
			newStdout = open("/dev/null", O_WRONLY);
		
		if(newStderr == -1)	//if not specified, use /dev/null for stderr
			newStderr = open("/dev/null", O_WRONLY);
		
		if(dup2(newStdin, 0) == -1)
			exitWithError("Error: Could not switch stdin file, exiting.\n");
		
		if(dup2(newStdout, 1) == -1)
			exitWithError("Error: Could not switch stdout file, exiting.\n");
		
		if(dup2(newStderr, 2) == -1)
			exitWithError("Error: Could not switch stderr file, exiting.\n");
		
		//fork
		int forkResult = fork();
		if(forkResult < 0)
			exitWithError("Error: Could not fork the process to daemonize it, exiting.\n");
		else if(forkResult > 0)
			//this is the parent process, exit
			exit(0);
		
		//get a new security identifier, this puts the fork()ed process into a new process group
		setsid();
		
		if(pidfile != 0)
		{
			pid_t pid = getpid();
			fprintf(pidfile, "%i\n", pid);
			fclose(pidfile);
		}
		
		//close all other fds, releasing them and associated resources
		for(int i = getdtablesize(); i > 2; --i)
			close(i);
	}
	return node::Start(argc, args);
}
#endif
