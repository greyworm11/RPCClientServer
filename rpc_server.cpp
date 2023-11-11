#pragma comment(lib, "rpcrt4.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include "solution.h"
using namespace std;

#define PORT "8000"
const unsigned block_size = 1048576;

handle_t handle;
int gOffset = 0;

int user_login(unsigned char* login, unsigned char* password)
{
	if (LogonUserA((char*)login, NULL,
		(char*)password, LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_WINNT40, &handle) == 0)
	{
		cout << "    - failed to connect with " << login << endl;
		return 1;
	}
	cout << "    + connected with " << login << endl;

	return 0;
}


void user_logout(void)
{
	RevertToSelf();
}

unsigned long server_to_host(const unsigned char* iFilename, unsigned char* buffer, unsigned long bfSize)
{

	cout << "    * sending file to client" << endl;

	if (ImpersonateLoggedOnUser(handle) == 0)
	{
		cout << "    - impersonating failed" << endl;
		return 1;
	}

	unsigned long size = 0;
	FILE* file = fopen((const char*)iFilename, "rb");

	if (file == NULL)
	{
		cout << "    - unable to open file..." << endl;
		if (errno == EACCES) {
			cout << "    - access denied..." << endl;
			user_logout();
			return 0;
		}
		else
		{
			cout << "    - error while sending file..." << endl;
			user_logout();
			return 0;
		}
	}

	fseek(file, gOffset, SEEK_SET);
	fread(buffer, sizeof(char), block_size, file);
	gOffset += block_size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size < (unsigned long)gOffset)
	{
		size += block_size - gOffset;
		gOffset = 0;
	}
	else
		size = block_size;

	fclose(file);

	cout << "    + data block sent" << endl;

	user_logout();

	return size;
}

FILE* fille;

int host_to_server(const unsigned char* iFilename, unsigned char* buffer, unsigned long bfSize, unsigned long start) {

	cout << "    * recieving file from client" << endl;

	if (ImpersonateLoggedOnUser(handle) == 0)
	{
		cout << "    - impersonating failed..." << endl;
		return 2;
	}
	if (start == 0)
	{
		fille = fopen((const char*)iFilename, "ab");
	}
	if (fille == NULL)
	{
		cout << "    - unable to open file..." << endl;
		if (errno == EACCES)
		{
			cout << "    - access denied..." << endl;
			user_logout();
			return -1;
		}
		else
		{
			cout << "    - error while receiving file..." << endl;
			user_logout();
			return 1;
		}
	}
	fseek(fille, start, SEEK_SET);
	fwrite(buffer, sizeof(char), bfSize, fille);
	cout << "    + block of data received " << endl;
	if (bfSize > 0 && bfSize < block_size) {
		fclose(fille);
		cout << "    + file copied " << endl;
	}
	user_logout();
	return 0;
}

void remove_file(const unsigned char* filename)
{

	cout << "    * deleting file with name '" << filename << "'" << endl;

	if (ImpersonateLoggedOnUser(handle) == 0)
	{
		cout << "    - impersonating failed..." << endl;
		return;
	}

	if (remove((const char*)filename) == -1)
	{
		if (errno == EACCES)
		{
			cout << "    - access denied... " << endl;
			user_logout();
			return;
		}
		else
		{
			cout << "    - error while deleting file... " << endl;
			user_logout();
			return;
		}

	}
	cout << "    + file deleted " << endl;
	user_logout();
}


int main()
{
	RPC_STATUS status = 0;

	status = RpcServerUseProtseqEpA(
		(RPC_CSTR)("ncacn_ip_tcp"),
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
		(RPC_CSTR)((const char*)PORT),
		NULL);

	if (status) exit(status);

	status = RpcServerRegisterIf2(
		solution_v1_0_s_ifspec,
		NULL,
		NULL,
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		(unsigned)-1,
		NULL);

	if (status) exit(status);

	cout << "server is on" << endl;
	cout << "listening on port: " << PORT << endl;

	status = RpcServerListen(
		1,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		FALSE);

	if (status) exit(status);
}

// Memory allocation function for RPC.
// The runtime uses these two functions for allocating/deallocating
// enough memory to pass the string to the server.
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

// Memory deallocation function for RPC.
void __RPC_USER midl_user_free(void* p)
{
	free(p);
}
