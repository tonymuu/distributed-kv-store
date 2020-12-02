/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file (Revised 2020)
 *
 * MP2 Starter template version
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

#define QUORUM 2
#define TOTAL 3
#define OPERATION_TIMEOUT 20

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "Message.h"
#include "Queue.h"

class Transaction {
public:
    // how many success does the transaction have (for quorum calculation)
    int successCount;
    // how many replies received
    int totalCount;
    // transaction id
    int txId;
    // timestamp this transaction was created
    int timestamp;
    // type of transaction, which overlaps partially with MessageType
    MessageType type;
    string key;
    string value;

    Transaction(MessageType _type, int timestamp);
};

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	// the value is serialized in the format of "value:timestamp:replicaType", should use Entry's converToString to serialize
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;
	// string delimiter
	string delimiter;
	// Transaction map
	map<int, Message>transMap;
	// maps txid => success reply count
	map<int, Transaction> txMap;

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	bool compareNode(const Node& first, const Node& second) {
		return first.nodeHashCode < second.nodeHashCode;
	}

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	// my functions
    void sendMessage(Address toAddr, Message msg);
    void updateTransactionMap();

    // message handlers
    void handleCreateMessage(Message msg);
    void handleUpdateMessage(Message msg);
    void handleReadMessage(Message msg);
    void handleDeleteMessage(Message msg);
    void handleReplyMessage(Message msg);
    void handleReadReplyMessage(Message msg);

	~MP2Node();
};

#endif /* MP2NODE_H_ */

/*
 * checkMessage:
 *   create message received: create kv pairs for my hashtable, log success/failure, send back reply if success
 *   read message received: read from my ht, log, send reply if success
 *   update/delete: perform action, log, send reply if success
 *   reply: count if quorum is reached (2), log success/fail
 *   read reply: count if quorum reached, log success/fail, return value
 *
 * For each client ops, create a transaction and save it in the txMap with txid (g_transId), increment too
 * During check message, when receive reply/replayread msg, update transaction accordingly (totalRcv vs successRcv) and
 * fail this transaction if no reach quorum
 *
 *
 * client ops:
 *   create: create a txid, findNodes for the given key, send messages to these nodes with txid
 *   update: similar to create but different message type
 *   read: similar
 *   delete: similar
 *
 *
 *
 *
 *
 * */