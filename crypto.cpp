#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <math.h>
#include "sha256.h"
#include "crypto.h"

const int PACKET_DATA_LENGTH = 32;
const int PACKET_CHECKSUM_LENGTH = 32;
const int PACKET_LENGTH = PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH;

char* OTP(char* output, unsigned long long* index, const unsigned long long amount);
char* xorCharArray(char* output, const char* first, const char* second, unsigned long long length);
unsigned long long unsignedLongLongRand();
char* longToCharArray(char* output, unsigned long long num, unsigned long long size);
unsigned long long charArrayToLong(const char* data, unsigned long long size);
bool charArrayEquals(const char* data1, const char* data2, unsigned long long size);
char* concat(char* output, const char* left, const char* right, unsigned long long sizel, unsigned long long sizer);
char* sha_256(char* output, const char* input, unsigned long long size);


// send command
ssize_t cwrite(int fd, const void *buf, size_t len) {
	srand (time(NULL));
	#ifdef _DEBUG
	std::cout << "Sending a message" << std::endl;
	#endif
	
	//long indexOfPad = random integer from 0 to size of OTP in bytes
	unsigned long long* indexOfPad = new unsigned long long;
	*indexOfPad = unsignedLongLongRand();
	//char* nonce = longToCharArray(&indexOfPad, PACKET_LENGTH);
	char nonce[PACKET_LENGTH];
	longToCharArray(nonce, *indexOfPad, PACKET_LENGTH);
	//send nonce to reciever
	int n;
	n = write(fd, nonce, PACKET_LENGTH);
	if (n < 0) {
		return -1;
	}
	//1
	//char* myChecksum = xor(OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH), OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH));
	char p1[PACKET_LENGTH];
	char p2[PACKET_LENGTH];
	OTP(p1, indexOfPad, PACKET_LENGTH);
	OTP(p2, indexOfPad, PACKET_LENGTH);
	char checksum[PACKET_LENGTH];
	xorCharArray(checksum, p1, p2, PACKET_LENGTH);
	//2
	//byte* response = response from receiver
	char buffer[PACKET_LENGTH];
	n = read(fd, buffer, PACKET_LENGTH);
	if (n < 0) {
		//error
		n = write(fd, "NO", 2);
		if (n < 0) {
			return -1;
		}
		return -1;
	}
	else if (n == 0) {
		//error - no message
		n = write(fd, "NO", 2);
		if (n < 0) {
			return -1;
		}
		return -1;
	}
	else if(!charArrayEquals(buffer, checksum, PACKET_LENGTH)) {
		//myChecksum != response
		return -1;
	}
	//send ok to reciever
	n = write(fd, "OK", 2);
	if (n < 0) {
		return -1;
	}
	//2.1
	//2.2
	//for(int bufferIndex = 0; bufferIndex < len; bufferIndex += PACKET_DATA_LENGTH) {
	for(int bufferIndex = 0; bufferIndex < len; bufferIndex += PACKET_DATA_LENGTH) {
		char* toSend = new char[PACKET_DATA_LENGTH];
		bzero(toSend, PACKET_DATA_LENGTH);
		for(int i = 0; i < PACKET_DATA_LENGTH && bufferIndex + i < len; i++) {
			toSend[i] = ((char*)buf)[bufferIndex + i];
		}
		
		#ifdef _DEBUG
		std::cout << "toSend: ";
		for(int i = 0; i < PACKET_DATA_LENGTH; i++) std::cout << toSend[i];
		std::cout << std::endl;
		#endif
		
		//char* key = OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH);
		char key[PACKET_LENGTH];
		OTP(key, indexOfPad, PACKET_LENGTH);
		#ifdef _DEBUG
		std::cout << "key: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << key[i];
		std::cout << std::endl;
		#endif
		
		//char* mac = sha-256(concat(toSend, key));
		char mac[PACKET_LENGTH];
		char tmp[PACKET_LENGTH];
		sha_256(mac, concat(tmp, toSend, key, PACKET_DATA_LENGTH, PACKET_CHECKSUM_LENGTH), PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "mac: ";
		for(int i = 0; i < PACKET_CHECKSUM_LENGTH; i++) std::cout << mac[i];
		std::cout << std::endl;
		#endif
		
		//char* message = concat(toSend, mac);
		char message[PACKET_LENGTH];
		concat(message, toSend, mac, PACKET_DATA_LENGTH, PACKET_CHECKSUM_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "message: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << message[i];
		std::cout << std::endl;
		#endif
		
		//char* pad = OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH);
		char pad[PACKET_LENGTH];
		OTP(pad, indexOfPad, PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "pad: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << pad[i];
		std::cout << std::endl;
		#endif
		
		//send xor(message, pad) to reciever 
		#ifdef _DEBUG
		std::cout << "Sending encrypted message" << std::endl;
		#endif
		char enc[PACKET_LENGTH];
		xorCharArray(enc, message, pad, PACKET_LENGTH);
		
		n = write(fd, enc, PACKET_LENGTH);
		
		if (n < 0) {
			return -1;
		}
		#ifdef _DEBUG
		std::cout << "Sent encrypted message" << std::endl;
		#endif
		n = read(fd, buffer, 2);
		buffer[2] = '\0';
		#ifdef _DEBUG
		std::cout << "Got confirmation: " << buffer << std::endl;
		#endif
		if (n < 0) {
			//error
			return -1;
		}
		else if (n == 0) {
			//error - no message
			return -1;
		}
		else if(buffer[0] != 'O' || buffer[1] != 'K') {
			//error - not ok
			#ifdef _DEBUG
			std::cout << "Not OK: " << (buffer[0] == 'O') << " " << (buffer[1] == 'K') << std::endl;
			#endif
			return -1;
		}
		//3
	}
	#ifdef _DEBUG
	std::cout << "Done sending encrypted message" << std::endl;
	#endif
	n = write(fd, "DONE", 4);
	return len;
}

// recv command
ssize_t cread(int fd, void *buf, size_t len) {
	#ifdef _DEBUG
	std::cout << "Recieving a message" << std::endl;
	#endif
	//1
	//wait for nonce from sender
	char buffer[PACKET_LENGTH];
	int n;
	n = read(fd, buffer, PACKET_LENGTH);
	if (n < 0) {
		//error
		return -1;
	}
	else if (n == 0) {
		//error - no message
		return -1;
	}
	
	#ifdef _DEBUG
	std::cout << "buffer: ";
	for(int i = 0; i < PACKET_LENGTH; i++) std::cout << (int)buffer[i];
	std::cout << std::endl;
	#endif
	
	//long indexOfPad;
	//indexOfPad = charArrayToLong(message recieved from sender);
	unsigned long long indexOfPad = charArrayToLong(buffer, PACKET_LENGTH);
	#ifdef _DEBUG
	std::cout << "indexOfPad: " << indexOfPad << std::endl;
	#endif
	
	char message[PACKET_LENGTH];
	OTP(message, &indexOfPad, PACKET_LENGTH);
	
	#ifdef _DEBUG
	std::cout << "message: ";
	for(int i = 0; i < PACKET_LENGTH; i++) std::cout << message[i];
	std::cout << std::endl;
	#endif
	
	//byte* pad = OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH);
	char pad[PACKET_LENGTH];
	OTP(pad, &indexOfPad, PACKET_LENGTH);
	
	#ifdef _DEBUG
	std::cout << "pad: ";
	for(int i = 0; i < PACKET_LENGTH; i++) std::cout << pad[i];
	std::cout << std::endl;
	#endif
	
	//byte* reply = xor(message, pad);
	char reply[PACKET_LENGTH];
	xorCharArray(reply, message, pad, PACKET_LENGTH);
	
	#ifdef _DEBUG
	std::cout << "reply: ";
	for(int i = 0; i < PACKET_LENGTH; i++) std::cout << reply[i];
	std::cout << std::endl;
	#endif
	
	#ifdef _DEBUG
	std::cout << "Sending reply" << std::endl;
	#endif
	//send reply to sender
	n = write(fd, reply, PACKET_LENGTH);
	#ifdef _DEBUG
	std::cout << "Sent reply" << std::endl;
	#endif
	if (n < 0) {
		return -1;
	}
	//2
	//2.1
	//recieve ok, if not retry, abort, ignore?
	#ifdef _DEBUG
	std::cout << "Waiting for confirmation" << std::endl;
	#endif
	n = read(fd, buffer, 2);
	buffer[2] = '\0';
	#ifdef _DEBUG
	std::cout << "Got confirmation: " << buffer << std::endl;
	#endif
	if (n < 0) {
		//error
		return -1;
	}
	else if (n == 0) {
		//error - no message
		return -1;
	}
	else if(buffer[0] != 'O' || buffer[1] != 'K') {
		//error - not ok
		#ifdef _DEBUG
		std::cout << "Not OK: " << (buffer[0] == 'O') << " " << (buffer[1] == 'K') << std::endl;
		#endif
		return -1;
	}
	//2.2
	//int amount_recv = 0;
	int amount_recv = 0;
	//while(more to recieve or amount_recv + PACKET_DATA_LENGTH < len) {
	#ifdef _DEBUG
	std::cout << "Recieving data" << std::endl;
	#endif
	while(amount_recv + PACKET_DATA_LENGTH < len) {
		//3
		//byte data[PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH] = message recieved from sender
		char data[PACKET_LENGTH];
		bzero(data, PACKET_LENGTH);
		#ifdef _DEBUG
		std::cout << "Waiting for data" << std::endl;
		#endif
		n = read(fd, data, PACKET_LENGTH);
		if (n < 0) {
			//error
			return -1;
		}
		else if (n == 0) {
			//done - no message
			return amount_recv;
		}
		else if (data[0] == 'D' && data[1] == 'O' && data[2] == 'N' && data[3] == 'E') {
			return amount_recv;
		}
		amount_recv += n;
		#ifdef _DEBUG
		std::cout << "Got data" << std::endl;
		#endif
		//byte* key = OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH);
		char key[PACKET_LENGTH];
		OTP(key, &indexOfPad, PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "key: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << key[i];
		std::cout << std::endl;
		#endif
	
		//pad = OTP(&indexOfPad, PACKET_DATA_LENGTH + PACKET_CHECKSUM_LENGTH);
		char pad[PACKET_LENGTH];
		OTP(pad, &indexOfPad, PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "pad: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << pad[i];
		std::cout << std::endl;
		#endif
		
		//byte* messsage = xor(data, pad);
		char message[PACKET_LENGTH];
		xorCharArray(message, data, pad, PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "message: ";
		for(int i = 0; i < PACKET_LENGTH; i++) std::cout << message[i];
		std::cout << std::endl;
		#endif
		
		//byte* checksum = data + PACKET_DATA_LENGTH;     checksum is last part after the data
		char* checksum = message + PACKET_DATA_LENGTH;
		
		#ifdef _DEBUG
		std::cout << "checksum: ";
		for(int i = 0; i < PACKET_CHECKSUM_LENGTH; i++) std::cout << checksum[i];
		std::cout << std::endl;
		#endif

		char mac[PACKET_LENGTH];
		char tmp[PACKET_LENGTH];
		sha_256(mac, concat(tmp, message, key, PACKET_DATA_LENGTH, PACKET_CHECKSUM_LENGTH), PACKET_LENGTH);
		
		#ifdef _DEBUG
		std::cout << "mac: ";
		for(int i = 0; i < PACKET_CHECKSUM_LENGTH; i++) std::cout << mac[i];
		std::cout << std::endl;
		#endif
		
		if(!charArrayEquals(checksum, mac, PACKET_CHECKSUM_LENGTH)) {
			//checksum != mac
			n = write(fd, "NO", 2);
			#ifdef _DEBUG
			std::cout << "Invalid checksum" << std::endl;
			#endif
			return -1;
		}
		n = write(fd, "OK", 2);
		if (n < 0) {
			return -1;
		}
		
		//copy message into buffer
		#ifdef _DEBUG
		std::cout << "message: " << std::flush;
		#endif
		for(int i = 0; i < PACKET_DATA_LENGTH; i++) {
			((char*)buf)[i] = message[i];
			#ifdef _DEBUG
			std::cout << message[i] << std::flush;
			#endif
		}
		#ifdef _DEBUG
		std::cout << std::endl;
		#endif
		buf += PACKET_DATA_LENGTH;
		
	}
	return amount_recv;	
}

char* OTP(char* output, unsigned long long* index, const unsigned long long amount) {
	std::ifstream pad("otp.key", std::ifstream::ate | std::ifstream::binary);
	pad.seekg(0,std::ios::end);
	pad.seekg(*index % pad.tellg());
	pad.read(output, amount);
	pad.close();
	*index += amount;
	return output;
}

char* xorCharArray(char* output, const char* first, const char* second, unsigned long long size) {
	for(unsigned long long i = 0; i < size; i++) {
		output[i] = first[i] ^ second[i];
	}
	return output;
}

unsigned long long unsignedLongLongRand() {
    unsigned long long rand1 = abs(rand());
    unsigned long long rand2 = abs(rand());
    rand1 <<= (sizeof(int)*8);   
    unsigned long long randULL = (rand1 | rand2);   
    return randULL;
}

char* longToCharArray(char* output, unsigned long long num, unsigned long long size) {
	for(int i = 0; i < sizeof(unsigned long long); i++){
		output[i] = num % (int)pow (2,(sizeof(char) * 8));
		num /= (int)pow(2,(sizeof(char) * 8));
	}
	return output;
}

unsigned long long charArrayToLong(const char* data, unsigned long long size){
	unsigned long long result = 0;
	for(int i = 0; i < size; i++){
		result += ((unsigned long long)(unsigned char)data[i]) * (unsigned long long)pow(2,i*(sizeof(char) * 8));
	}
	return result;
}

bool charArrayEquals(const char* data1, const char* data2, unsigned long long size) {
	for(int i = 0; i < size; i++) {
		if(data1[i] != data2[i]) return false;
	}
	return true;
}

char* concat(char* output, const char* left, const char* right, unsigned long long sizel, unsigned long long sizer) {
	for(int i = 0; i < sizel; i++) {
		output[i] = left[i];
	}
	for(int i = 0; i < sizer; i++) {
		output[sizel + i] = right[i];
	}
	return output;
}

char* sha_256(char* output, const char* input, unsigned long long size) {
	return sha256(output, input, size);
}

//For testing     
/*
int main(int argc, char *argv[]){
	srand (time(NULL));
	unsigned long long* indexOfPad = new unsigned long long;
	std::cout << "Index of pad = " << *indexOfPad << std::endl;
	*indexOfPad = unsignedLongLongRand();
	std::cout << "Index of pad = " << *indexOfPad << std::endl;
	char* nonce = longToCharArray(*indexOfPad, PACKET_LENGTH);
	std::cout << "nonce = " << charArrayToLong(nonce, PACKET_LENGTH) << std::endl;
	char* pad1 = OTP(indexOfPad,10);
	char* pad2 = OTP(indexOfPad,10);
	char* padx = xorCharArray(pad1, pad2, 10);
	for(int i = 0; i < 10; i++) {
		std::cout << (unsigned int)(unsigned char)pad1[i] << " ";
	}
	std::cout << std::endl;
	for(int i = 0; i < 10; i++) {
		std::cout << (unsigned int)(unsigned char)pad2[i] << " ";
	}
	std::cout << std::endl;
	for(int i = 0; i < 10; i++) {
		std::cout << (unsigned int)(unsigned char)padx[i] << " ";
	}
	std::cout << std::endl;
	std::cout << charArrayEquals(padx, pad2, 10) << std::endl;
	char* padc = concat(pad1, pad2, 10, 10);
	for(int i = 0; i < 20; i++) {
		std::cout << (unsigned int)(unsigned char)padc[i] << " ";
	}
	std::cout << std::endl;
	std::cout << sha_256(padc) << std::endl;
	std::cout << sha_256(pad1) << std::endl;
	std::cout << sha_256(padc) << std::endl;
	delete nonce;
	delete indexOfPad;
}
//*/