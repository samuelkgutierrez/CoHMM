#ifndef REDISSHMEM_HPP
#define REDISSHMEM_HPP

#include <hiredis.h>
#include <cstring>

#include "2DKriging.hpp"

#include <iostream>

const unsigned int fieldBlockSize = 32;
const unsigned int maxKeyLength = 32;
const char rshmem_tag[] = "SWIFTT_TEST";

void buildKey(char * key, int curStep, int curPhase, int ID, const char * tag);
//Determine how many blocks the field has been chunked in to
unsigned int getNumBlocks(int dimX, int dimY);

template <typename T> bool putBlocks(T * field, int dimX, int dimY, int curStep, int curPhase, redisContext * redis, const char * tag)
{
	unsigned int nBlocks = getNumBlocks(dimX, dimY);
	char keyBuffer[maxKeyLength];

	//Push the first nBlocks-1 blocks as they are easy
	for(unsigned int i = 0; i < (nBlocks - 1); i++)
	{
		//Build key
		buildKey(keyBuffer, curStep, curPhase, i, tag);
		//Do a redis push
		redisReply *reply;
		reply = (redisReply *) redisCommand(redis, "SET %s %b", keyBuffer, &field[i*fieldBlockSize], sizeof(T)*fieldBlockSize);
		freeReplyObject(reply);
	}
	//See if last block is full
	int i = nBlocks - 1;
	buildKey(keyBuffer, curStep, curPhase, i, tag);
	if( (dimX*dimY) % fieldBlockSize == 0)
	{
		//It was, so same as above
		//Do a redis push
		redisReply *reply;
		reply = (redisReply *) redisCommand(redis, "SET %s %b", keyBuffer, &field[i*fieldBlockSize], sizeof(T)*fieldBlockSize);
		freeReplyObject(reply);
	}
	else
	{
		//It was not, so only copy what we need
		unsigned int lastBlock = (dimX*dimY) % fieldBlockSize;
		//Do a redis push
		redisReply *reply;
		reply = (redisReply *) redisCommand(redis, "SET %s %b", keyBuffer, &field[i*fieldBlockSize], sizeof(T)*lastBlock);
		freeReplyObject(reply);
	}
	//Success
	return true;
}

template <typename T> bool getBlocks(T * field, int dimX, int dimY, int curStep, int curPhase, redisContext * redis, const char * tag)
{
	unsigned int nBlocks = getNumBlocks(dimX, dimY);
	char keyBuffer[maxKeyLength];
	size_t blockSize = sizeof(T) * fieldBlockSize;

	//Push the first nBlocks-1 blocks as they are easy
	for(unsigned int i = 0; i < (nBlocks - 1); i++)
	{
		//Build key
		buildKey(keyBuffer, curStep, curPhase, i, tag);
		//Do a redis pull
		redisReply *reply;
		reply = (redisReply *) redisCommand(redis, "GET %s", keyBuffer);
		assert(reply->type == REDIS_REPLY_STRING);
		memcpy(&field[i*fieldBlockSize], reply->str, blockSize);
		freeReplyObject(reply);
	}
	//Last one is slightly easier with GET
	//Do a redis pull
	int i = nBlocks - 1;
	buildKey(keyBuffer, curStep, curPhase, i, tag);
	redisReply *reply;
	reply = (redisReply *) redisCommand(redis, "GET %s", keyBuffer);
	assert(reply->type == REDIS_REPLY_STRING);
	if((dimX*dimY) % fieldBlockSize != 0)
	{
		blockSize = ((dimX*dimY) % fieldBlockSize) * sizeof(T);
	}
	memcpy(&field[i*fieldBlockSize], reply->str, blockSize);
	//Success
	return true;
}


template <typename T> bool putSingle(T * item, int curStep, int curPhase, int ID, redisContext * redis, const char * tag)
{
	char keyBuffer[maxKeyLength];
	//Get Key
	buildKey(keyBuffer, curStep, curPhase, ID, tag);
	//Do a redis push
	redisReply *reply;
	reply = (redisReply *) redisCommand(redis, "SET %s %b", keyBuffer, item, sizeof(T));
	freeReplyObject(reply);
	//Success
	return true;
}

template <typename T> bool getSingle(T * item, int curStep, int curPhase, int ID, redisContext * redis, const char * tag)
{
	char keyBuffer[maxKeyLength];
	//Get Key
	buildKey(keyBuffer, curStep, curPhase, ID, tag);
	redisReply *reply;
	reply = (redisReply *)redisCommand(redis, "GET %s", keyBuffer);
	assert(reply->type == REDIS_REPLY_STRING);
	memcpy(item, reply->str, sizeof(T));
	freeReplyObject(reply);
	//Success
	return true;
}

#endif

