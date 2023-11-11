#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <conio.h>
#include <cctype>
#include <string>
#include "solution.h"
#pragma comment(lib, "rpcrt4.lib")

const size_t blockSize = 1048576;

unsigned char* szStringBinding = NULL;

void connect(const std::string& IP, const std::string& port = "8000")
{
	RPC_STATUS status;

	status = RpcStringBindingComposeA(
		NULL,
		(RPC_CSTR)("ncacn_ip_tcp"),
		(RPC_CSTR)(IP.c_str()),
		(RPC_CSTR)(port.c_str()),
		NULL,
		&szStringBinding
	);

	if (status)
		exit(status);

	status = RpcBindingFromStringBindingA(szStringBinding, &handle_);

	if (status)
		exit(status);
}

int login(void)
{
	std::string username;
	std::string password;

	while (1)
	{
		std::cout << "enter username" << std::endl;
		std::getline(std::cin, username);
		std::cout << "enter password" << std::endl;
		std::getline(std::cin, password);

		if (user_login((unsigned char*)username.c_str(), (unsigned char*)password.c_str()))
			std::cout << "connection failed..." << std::endl << std::endl;
		else
			break;
	}
	std::cout << "user " << username << " connected successfully!" << std::endl;
	return 0;
}

void copy_server_to_host(const std::string& filename)
{
	unsigned char* out = new unsigned char[blockSize];
	memset(out, 0, blockSize);

	unsigned size = server_to_host((const unsigned char*)filename.c_str(), out, blockSize);

	if (size != 0)
	{
		FILE* fout = fopen(filename.c_str(), "wb");
		fwrite(out, sizeof(char), size, fout);

		do
		{
			memset(out, 0, blockSize);
			size = server_to_host((const unsigned char*)filename.c_str(), out, blockSize);
			if (size == 0)
				break;
			fwrite(out, sizeof(char), size, fout);

		} while (size == blockSize);

		fclose(fout);
	}
	else
		std::cout << "no such file or this file is not avaliable..." << std::endl;

	delete[] out;
}

void copy_host_to_server(const std::string& filename)
{
	FILE* file = fopen((const char*)filename.c_str(), "rb");
	fseek(file, 0, SEEK_END);
	unsigned size = ftell(file);
	fseek(file, 0, SEEK_SET);

	unsigned int sent = 0;
	unsigned counter = 0;

	while (1)
	{
		if (size - sent >= blockSize)
		{
			unsigned char* in = new unsigned char[blockSize + 4];
			fseek(file, counter, SEEK_SET);
			fread(in, sizeof(char), blockSize, file);
			if (host_to_server((const unsigned char*)filename.c_str(), (unsigned char*)in, blockSize, counter) != 0)
			{
				std::cout << "permission denied..." << std::endl;
				return;
			}
			delete[] in;
			sent += blockSize;
			counter += blockSize;
		}
		else
		{
			unsigned char* in = new unsigned char[blockSize];
			fread(in, sizeof(char), size - sent, file);
			fclose(file);
			host_to_server((const unsigned char*)filename.c_str(), (unsigned char*)in, size - sent, counter);
			delete[] in;
			break;
		}
	}

}

void print_commands(void)
{
	std::cout << "choose command: " << std::endl;
	std::cout << "    1. delete file on server" << std::endl;
	std::cout << "    2. copy file from server to the host" << std::endl;
	std::cout << "    3. copy file from host to server" << std::endl;
	std::cout << "    4. exit" << std::endl;
}

const std::string get_filename(void)
{
	static std::string filename;
	std::cout << "enter file name" << std::endl;
	std::getline(std::cin, filename);

	return filename;
}

void switch_command(void)
{
	static std::string filename;
	static std::string str;
	std::getline(std::cin, str);

	unsigned number = std::stoi(str);
	switch (number % 4)
	{
	case 0:
		user_logout();
		exit(1);
		break;
	case 1:
		remove_file((const unsigned char*)get_filename().c_str());
		break;
	case 2:
		copy_server_to_host(get_filename());
		break;
	case 3:
		copy_host_to_server(get_filename());
		break;
	default:
		break;
	}
}

int main()
{
	RPC_STATUS status;

	std::cout << "Enter IP" << std::endl;
	std::string ip = "192.168.56.1";
	std::getline(std::cin, ip);
	connect(ip);
	login();

	while (1)
	{
		print_commands();
		switch_command();
	}

	status = RpcStringFreeA(&szStringBinding);

	if (status)
		exit(status);

	status = RpcBindingFree(&handle_);

	if (status)
		exit(status);
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
