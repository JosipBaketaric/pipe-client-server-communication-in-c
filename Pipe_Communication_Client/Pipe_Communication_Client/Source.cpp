#define _CRT_SECURE_NO_WARNINGS

#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFFSIZE 512
HANDLE hThread[2];	
BOOL myWaite = FALSE;
HANDLE hPipe1;

DWORD WINAPI ThreadForSending(LPVOID);
DWORD WINAPI ThreadForRecieving(LPVOID);
HANDLE ConnectToPipeServer();
VOID myWaiteFunction();

int _tmain(/*int argc, TCHAR *argv[]*/)
{
	DWORD dwThreadId = 0;
	
	hPipe1 = ConnectToPipeServer();

	//create a thread for sending
	hThread[0] = CreateThread(
		NULL,					//no security attributes
		0,						//default stack size
		ThreadForSending,		//thread rutine
		(LPVOID)hPipe1,			//thread parameter
		0,						//not suspended
		&dwThreadId);			//returns thread ID

	//creates a thread for recieving
	hThread[1] = CreateThread(
		NULL,					//no security attributes
		0,						//default stack size
		ThreadForRecieving,		//thread rutine
		(LPVOID)hPipe1,			//thread parameter
		0,						//not suspended
		&dwThreadId);			//returns thread ID*/
	WaitForMultipleObjects(2,hThread ,TRUE, INFINITE);
	WaitForMultipleObjects(1, hThread, TRUE, INFINITE);
	CloseHandle(hPipe1);
	return 0;
}

HANDLE ConnectToPipeServer()
{
	HANDLE hPipe;
	BOOL fSuccess = FALSE;
	DWORD dwMode;
	LPTSTR lpszPipeName = TEXT("\\\\.\\pipe\\mynamedpipe");

	//try to open a named pipe

	while (1)
	{
		hPipe = CreateFile(
			lpszPipeName,	//pipe name
			GENERIC_READ |	//read and write acces
			GENERIC_WRITE,
			0,				//no sharing
			NULL,			//default security attributes
			OPEN_EXISTING,	//open existing pipe
			0,				//default attributes
			NULL);			//no template file

							//Break if the pipe handle is valid
		if (hPipe != INVALID_HANDLE_VALUE)
			break;

		//Exit if an error occures other than ERROR_PIPE_BUSY
		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			_tprintf(TEXT("\tSystem: Could not open pipe. GLE=%d\n"), GetLastError());
			return INVALID_HANDLE_VALUE;
		}

		//All pipe instances are busy, wait 20 seconds.
		if (!WaitNamedPipe(lpszPipeName, 20000))
		{
			printf("\tSystem: Could not open pipe: 20 second wait timed out.");
			return INVALID_HANDLE_VALUE;
		}
	}

	//The pipe connected. Change to message-read mode.
	dwMode = PIPE_READMODE_MESSAGE;

	fSuccess = SetNamedPipeHandleState(
		hPipe,		//pipe handle
		&dwMode,	//new pipe mode
		NULL,		//don't set maximum bytes
		NULL);		//don't set maximum time

	if (!fSuccess)
	{
		_tprintf(TEXT("\tSystem: SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
		return INVALID_HANDLE_VALUE;
	}
	return hPipe;
}

DWORD WINAPI ThreadForSending(LPVOID lpvParam)
{
	lpvParam = (HANDLE)lpvParam;
	DWORD cbToWrite, cbWritten;
	LPTSTR lpvMessage;
	BOOL fSuccess = FALSE;
	char temp[BUFFSIZE];
	//temp = (char*)malloc(sizeof(BUFFSIZE));
	LPTSTR userName;

	//set userName
	do
	{
		_tprintf(TEXT("\nType User Name (max 20 char): "));
		scanf("%[^\n]%*c", temp);			//set delimiter to be \n	scanf("%[^\n]%*c", temp);
		userName = (LPTSTR)temp;
		cbToWrite = (lstrlen(userName) + 1)*sizeof(TCHAR);
	} while (cbToWrite > 21);

	myWaite = TRUE;
	CancelIoEx(hPipe1, NULL);	
	fSuccess = WriteFile(
		lpvParam,	//pipe handle
		userName,	//message
		cbToWrite,	//message length
		&cbWritten,	//bytes written
		NULL);		//not overlapped
	myWaite = FALSE;

	if (!fSuccess)
	{
		_tprintf(TEXT("\tSystem: WriteFile to pipe failed. GLE=%d\n"), GetLastError());
		return -1;
	}

	//loop for sending
	while (1)
	{	
		
		_tprintf(TEXT("\nMessage: "));
		scanf("%[^\n]%*c", temp);			//set delimiter to be \n	scanf("%[^\n]%*c", temp);

		lpvMessage = (LPTSTR)temp;
		cbToWrite = (lstrlen(lpvMessage) + 1)*sizeof(TCHAR);
		//_tprintf(TEXT("\tSystem: Sending %d byte message: \"%s\"\n"), cbToWrite, lpvMessage);
		
		myWaite = TRUE;
		CancelIoEx(hPipe1, NULL);
		fSuccess = WriteFile(
			hPipe1,			//pipe handle
			lpvMessage,		//message
			cbToWrite,		//message length
			&cbWritten,		//bytes written
			NULL);			//not overlapped
		
		myWaite = FALSE;

		if (!fSuccess)
		{
			_tprintf(TEXT("\tSystem: WriteFile to pipe failed. GLE=%d\n"),GetLastError());
			return -1;
		}	
		//printf("\n\tSystem: Message sent to server.\n");
	}
	return 0;
}




DWORD WINAPI ThreadForRecieving(LPVOID lpvParam)
{
	
	lpvParam = (HANDLE)lpvParam;
	BOOL fSuccess = FALSE;
	DWORD cbRead;
	TCHAR chBuff[BUFFSIZE];
	
	while (1)
	{
		do
		{	
			fSuccess = ReadFile(
				hPipe1,					//pipe handle
				chBuff,					//buffer to recive reply
				BUFFSIZE*sizeof(TCHAR),	//size of buffer
				&cbRead,				//number of bytes read
				NULL);					//not overlapped

			if (myWaite == TRUE)
				myWaiteFunction();

			if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
				break;
			_tprintf(TEXT("\tServer:%s"), chBuff);	
		} 
		while (!fSuccess);	//repeat loop if ERROR_MORE_DATA
	}
		if (!fSuccess)
		{
			_tprintf(TEXT("\tSystem: ReadFile from pipe failed. GLE=%d\n"), GetLastError());
			return -1;
		}	
}


VOID myWaiteFunction()
{
	printf("\tSystem: Locked->");
	while (myWaite == TRUE)
	{ }
	printf("Unlocked\n");
}