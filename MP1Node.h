/**********************************
 * FILE NAME: MP1Node.h
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class. (Revised 2020)
 *
 *  Starter code template
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5
#define TGOSSIP 5

#define GOSSIP_SIZE 3

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	bool shouldDeleteMember;
	char NULLADDR[6];
	long timeLastGossip;
	long localTimeStamp;

public:
	/**
	 * Message Types
	 */
	enum MsgTypes {
	    JOINREQ,
	    JOINREP,
	    UPDATEREQ,
	    UPDATEREP,
	    DUMMYLASTMSGTYPE
	};

	/**
	 * STRUCT NAME: MessageHdr
	 *
	 * DESCRIPTION: Header and content of a message
	 */
	struct MessageHdr {
		MsgTypes msgType;
	};

	MP1Node(Params *params, EmulNet *emul, Log *log, Address *address);
	MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
    virtual ~MP1Node();

    // My functions
    void sendJoinRepMsg(Address *toAddr);
    void sendGossip(Address *toAddr);
    void setIdAndPortFromAddress(Address addr, int *id, short *port);
    Address createAddressFromIdAndPort(int id, short port);
    bool areAddressesEqual(Address addr1, Address addr2);
    map<int, MemberListEntry*> createMemberlistMap();
    void serializeMemberList(MessageHdr* msg);
    void deserializeAndUpdateMemberList(char *data, int size);
    MemberListEntry* getNodeFromMemberListTable(int id);
};

#endif /* _MP1NODE_H_ */
