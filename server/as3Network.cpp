/*
 * This file is part of the as3server Project. http://www.as3server.org
 *
 * Copyright (c) 2010 individual as3server contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

#include "as3Network.h"
#include <iostream>
using namespace std;

struct addrinfo si_depth, si_rgb, si_data;

pthread_t data_thread, data_in_thread, data_out_thread;

SOCKET data_socket = INVALID_SOCKET;
SOCKET data_client_socket = INVALID_SOCKET;

PCSTR conf_port_data = "6001";

int die = 0;
int data_child;
int psent = 0;

// callback handler
void as3Network::onConnect(void (*eConnectHandler)())
{
  this->eConnect = eConnectHandler;
}

/* 
 * sendMessage (Debug message)
 * sending: 3 0 msg
 * 3 -> Server message
 * 0 -> Debug message
 */
void as3Network::sendMessage(const char *data) {
	std::string _msg = data;
	int len = sizeof(_msg);
	int first = 3;
	int second = 0;
	int m_len = 1 + 1 + sizeof(int);
	unsigned char *msg = new unsigned char[len + m_len];
	memcpy(msg, &first, 1);
	memcpy(msg + 1, &second, 1);
	memcpy(msg + 2, &len, sizeof(int));
	memcpy(msg + m_len, data, len);
	sendData(msg,len+m_len);
	delete [] msg;
}

/* 
 * sendMessage
 * sending: first second value
 * first -> Ex: 3 (Server message)
 * second -> Ex: 1 (User detected)
 * value -> Ex: 1 (User id)
 */
void as3Network::sendMessage(int first, int second, int value) {
	unsigned char buff[2 + sizeof(int) * 2];
	buff[0] = first;
	buff[1] = second;
	int size = sizeof(int);
	memcpy(buff+2, &size, sizeof(int));
	memcpy(buff+6, &value, sizeof(int));
	sendData(buff, 2 + sizeof(int) * 2);
}

/* 
 * sendMessage
 * sending: first second data
 * first -> Ex: 0 (Camera message)
 * second -> Ex: 0 (Depth data)
 */
void as3Network::sendMessage(int first, int second, unsigned char *data, int len) {
	int m_len = 1 + 1 + sizeof(int);
	unsigned char *msg = new unsigned char[m_len + len];
	memcpy(msg, &first, 1);
	memcpy(msg + 1, &second, 1);
	memcpy(msg + 2, &len, sizeof(int));
	memcpy(msg + m_len, data, len);
	sendData(msg,m_len + len);
	delete [] msg;
}

/* 
 * sendData
 */
int as3Network::sendData(unsigned char *buffer, int length){
	if(data_client_socket != INVALID_SOCKET) {
		return send(data_client_socket, (char*)buffer, length, 0);
	}
	return 0;
}

/* 
 * getData
 */
int as3Network::getData(unsigned char *buffer, int length){
	if(data_client_socket != INVALID_SOCKET) {
		return recv(data_client_socket, (char*)buffer, 1024, 0);
	}
	return 0;	
}

void as3Network::send_policy_file(int child){
	if(psent == 0){
		int n;
		char str[] = "<?xml version='1.0'?><!DOCTYPE cross-domain-policy SYSTEM '/xml/dtds/cross-domain-policy.dtd'><cross-domain-policy><site-control permitted-cross-domain-policies='all'/><allow-access-from domain='*' to-ports='*'/></cross-domain-policy>\n";
		//n = write(child,str , 237);
		if ( n < 0 || n != 237)
		{
			fprintf(stderr, "Error on write() for depth (%d instead of %d)\n",n, 237);
			//break;
		}
		psent = 1;
	}
}

void *as3Network::network_data(void *pv) {
	thread_fun_args* tf_args = static_cast<thread_fun_args*>(pv) ;
    as3Network* This = tf_args->This ;
    void* args = tf_args->actual_arg ;

	while ( !die )
	{
		printf("### Wait data client\n");
		data_client_socket = accept(data_socket, NULL, NULL);
		if (data_client_socket == INVALID_SOCKET) {
			printf("Error on accept() for data, exit data thread. %d\n", WSAGetLastError());
			closesocket(data_socket);
			WSACleanup();
			break;
		}
		
		printf("### Got data client\n");
		//This->send_policy_file(depth_child);
	  	if(This->eConnect)
	  	{
	    	This->eConnect();
	  	}
	}
	
	delete tf_args ;
	return NULL;
}

int as3Network::init() {
	die = 0;
	initServer(si_data, conf_port_data, &data_socket, "DATA");

	if (pthread_create(&data_thread, NULL, &as3Network::network_data, new thread_fun_args(this,0)))
	{
		fprintf(stderr, "Error on pthread_create() for DATA\n");
		return -1;
	}
	return 0;
	
}

int as3Network::initServer(addrinfo si_type, PCSTR conf_port, SOCKET *the_socket, PCSTR label){
	ZeroMemory(&si_type, sizeof (si_type));

	si_type.ai_family = AF_INET;
	si_type.ai_socktype = SOCK_STREAM;
	si_type.ai_protocol = IPPROTO_TCP;
	si_type.ai_flags = AI_PASSIVE;
	
   // Resolve the local address and port to be used by the server
	struct addrinfo *result = NULL;	

	int iResult = getaddrinfo(NULL, conf_port, &si_type, &result);
	if (iResult != 0) {
		printf("%s: getaddrinfo failed: %d\n", label, iResult);
		WSACleanup();
		return 1;
	}

	*the_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (*the_socket == INVALID_SOCKET) {
		printf("%s: socket failed [%ld]\n", label, WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(*the_socket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("%s: bind failed: %d\n", label, WSAGetLastError());
		freeaddrinfo(result);
		closesocket(*the_socket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if ( listen(*the_socket, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "%s: listen failed [%ld]\n", label, WSAGetLastError() );
		closesocket(*the_socket);
		WSACleanup();
		return 1;
	}

	return 0;
}

void as3Network::close_connection() {
	die = 1;
	if ( data_socket != INVALID_SOCKET )
		closesocket(data_socket);
}
