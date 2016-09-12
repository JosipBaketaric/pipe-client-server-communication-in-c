#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h> //generic functions
#include <string.h>

#define BUFFSIZE 512
#define MAX_USERS 20
HANDLE hThread[MAX_USERS];
char *UserList[MAX_USERS];
int UserCounter;
BOOL myLock = TRUE;
	
DWORD WINAPI InstanceThread(LPVOID);
void GetAnswerToRequest(LPTSTR, LPTSTR, LPDWORD, TCHAR*);
void myLockFunction();

int _tmain(int argc, char **argv)
{
	HANDLE hPipe = INVALID_HANDLE_VALUE;
	BOOL fConnected = FALSE;
	DWORD dwThreadId = 0;
	//HANDLE hThread = NULL;
	LPSTR lpszPipeName = "\\\\.\\pipe\\mynamedpipe";
	UserCounter = 0;
	
	//main loop creates pipe instance and waits for connection
	//after the client connects, program creates communication thread and waits for next client
	for (;;)
	{
		_tprintf(TEXT("\nPipe Server: Main thread awaiting client connection on %s\n"), lpszPipeName);
		hPipe = CreateNamedPipe(
			lpszPipeName,				//Pipe name
			PIPE_ACCESS_DUPLEX,			//read-write access
			PIPE_TYPE_MESSAGE |			//message type pipe
			PIPE_READMODE_MESSAGE |		//message-read mode
			PIPE_WAIT,					//blocking mode
			PIPE_UNLIMITED_INSTANCES,	//max instances
			BUFFSIZE,					//output buffer size
			BUFFSIZE,					//input buffer size
			0,							//client time-out
			NULL);						//default security attribute

		if (hPipe == INVALID_HANDLE_VALUE)
		{
			_tprintf(TEXT("Creating named pipe failed, GLE=%d. \n"), GetLastError());
			return -1;
		}
		
		//Wait for client to connect
		fConnected = ConnectNamedPipe(hPipe, NULL) ?
			TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected)
		{
			printf("Client connected, creating thread \n");

			//Creating a thread for client
			hThread[UserCounter] = CreateThread(
				NULL,								//security attributes
				0,									//default stack size
				InstanceThread,						//thread rutine
				(LPVOID)hPipe,					//thread parametar
				0,									//stack size default
				&dwThreadId);						//returns thread id

			if (hThread == NULL)
			{
				_tprintf(TEXT("Failed to create thread, GLE=%d. \n"), GetLastError());
				return -1;
			}

			else
			{
				myLockFunction();
				UserCounter++;
			}
									//CloseHandle(hThread);
		}

		//if failed to connect close pipe
		else CloseHandle(hPipe);
	}

	return 0;
}

//Function for reading and for replying to a client
DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
	int UserIdNumber = UserCounter;
	myLock = FALSE;
	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFFSIZE*sizeof(TCHAR));
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFFSIZE*sizeof(TCHAR));

	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;
	TCHAR* userName = (TCHAR*)HeapAlloc(hHeap, 0, 20*sizeof(TCHAR));

	//ERROR CHECKING

	if (lpvParam == NULL)
	{
		printf("Error - Pipe Server failure:\n");
		printf("	InstanceThread got an unexpected NULL value in npvParam. \n");
		printf("	InstanceThread exitting. \n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL)
	{
		printf("Error - Pipe Server Failure:\n");
		printf("	InstanceThread got an unexpected NULL at heap allocation.\n");
		printf("	InstanceThread exitting.\n");
		if (pchReply != NULL) HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL)
	{
		printf("Error - Pipe Server Failure.\n");
		printf("	InstanceThread got an unexpected NULL at heap allocation.\n");
		printf("	InstanceThread Exitting.\n");
		if (pchRequest != NULL) HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	hPipe = (HANDLE)lpvParam;
	//set userName
	fSuccess = ReadFile(
		hPipe,				//Handle to pipe
		userName,					//buffer to recieve data
		BUFFSIZE*sizeof(TCHAR),		//size of buffer
		&cbBytesRead,				//number of bytes read
		NULL);						//not overlaped I/O
	//save users nick to global variable
	UserList[UserIdNumber] = userName;

	//loop for reading
	while (1)
	{
		fSuccess = ReadFile(
			hPipe,				//Handle to pipe
			pchRequest,					//buffer to recieve data
			BUFFSIZE*sizeof(TCHAR),		//size of buffer
			&cbBytesRead,				//number of bytes read
			NULL);						//not overlaped I/O

		if (!fSuccess || cbBytesRead == 0)
		{
			if (GetLastError() == ERROR_BROKEN_PIPE)
			{
				_tprintf(TEXT("InstanceThread: Client disconnected.\n"), GetLastError());
				UserList[UserIdNumber] = NULL;
			}
			else
			{
				_tprintf(TEXT("InstanceThread: ReadFile failed, GLE=%d.\n"), GetLastError());
			}
			break;
		}

		//process the incoming message
		int UserPrintCounter = 0;
		CHAR* sysReply;
		BOOL NoUsers = TRUE;
		// if users call =show users
		if (_tcscmp(pchRequest,"=show users") == 0)
		{
			while (UserPrintCounter <= UserCounter)
			{
				if (UserPrintCounter != UserIdNumber && UserList[UserPrintCounter] != NULL)
				{
					sysReply = UserList[UserPrintCounter];
					//write the reply to the pipe
					fSuccess = WriteFile(
						hPipe,					//handle to pipe
						sysReply,				//buffer to write from
						BUFFSIZE*sizeof(TCHAR),	//number of bytes to write
						&cbWritten,				//number of bytes writteen
						NULL);					//not overlaped I/O
					NoUsers = FALSE;
				}
				sysReply = "";
				UserPrintCounter++;
			}	
			if (NoUsers == TRUE)
			{
				sysReply = "No users online.";
				fSuccess = WriteFile(
					hPipe,					//handle to pipe
					sysReply,				//buffer to write from
					BUFFSIZE*sizeof(TCHAR),	//number of bytes to write
					&cbWritten,				//number of bytes writteen
					NULL);					//not overlaped I/O
			}
			UserPrintCounter = 0;
		}
		else if (_tcscmp(pchRequest, "=help") == 0)
		{
			sysReply = " 1) Do not press enter if you haven't written anything or program will crash\n\t2) Type '=show users' to see who else is online";
			fSuccess = WriteFile(
				hPipe,					//handle to pipe
				sysReply,				//buffer to write from
				BUFFSIZE*sizeof(TCHAR),	//number of bytes to write
				&cbWritten,				//number of bytes writteen
				NULL);					//not overlaped I/O
		}
		
		else
		{
			GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes, userName);
			/*
			//write the reply to the pipe
			fSuccess = WriteFile(
				hPipe,	//handle to pipe
				pchReply,		//buffer to write from
				cbReplyBytes,	//number of bytes to write
				&cbWritten,		//number of bytes writteen
				NULL);			//not overlaped I/O

			if (!fSuccess || cbReplyBytes != cbWritten)
			{
				_tprintf(TEXT("InstanceThread WriteFile failed, GLE.%d\n"), GetLastError());
				break;
			}
			*/
		}
		
	}

	//Flush the pipe to allow the client to read the pipe's contents

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	printf("InstanceThread Exitting.\n");
	return 1;	
}

//print client request
VOID GetAnswerToRequest(LPTSTR pchRequest, LPTSTR pchReply, LPDWORD pchBytes, TCHAR* userName)
{
	_tprintf(TEXT ("%s:\"%s\"\n"),userName, pchRequest);

	//check if outgoing message is not to long for the buffer
	if (FAILED(StringCchCopy(pchReply, BUFFSIZE, TEXT("Message recieved\n"))))
	{
		*pchBytes = 0;
		pchReply[0] = 0;
		printf("StringCchCopy failed, no outgoing message.\n");
		return;
	}
	*pchBytes = (lstrlen(pchReply) + 1)*sizeof(TCHAR);
}


void myLockFunction()
{
	while(myLock == TRUE)
	{ }
}