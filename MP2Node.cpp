/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition (Revised 2020)
 *
 * MP2 Starter template version
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
	this->delimiter = "::";
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
}

/**
* FUNCTION NAME: updateRing
*
* DESCRIPTION: This function does the following:
*                 1) Gets the current membership list from the Membership Protocol (MP1Node)
*                    The membership list is returned as a vector of Nodes. See Node class in Node.h
*                 2) Constructs the ring based on the membership list
*                 3) Calls the Stabilization Protocol
*/
void MP2Node::updateRing() {
	/*
     * Implement this. Parts of it are already implemented
     */
    vector<Node> curMemList;
    bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());

	ring = curMemList;
	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	stabilizationProtocol();
}

/**
* FUNCTION NAME: getMembershipList
*
* DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
*                 i) generates the hash code for each member
*                 ii) populates the ring member in MP2Node class
*                 It returns a vector of Nodes. Each element in the vector contain the following fields:
*                 a) Address of the node
*                 b) Hash code obtained by consistent hashing of the Address
*/
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
* FUNCTION NAME: hashFunction
*
* DESCRIPTION: This functions hashes the key and returns the position on the ring
*                 HASH FUNCTION USED FOR CONSISTENT HASHING
*
* RETURNS:
* size_t position on the ring
*/
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
* FUNCTION NAME: clientCreate
*
* DESCRIPTION: client side CREATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientCreate(string key, string value) {
    // start a transaction
    Transaction transaction(CREATE);
    transaction.key = key;
    transaction.value = value;
    txMap.emplace(transaction.txId, transaction);

    // send messages to all nodes who should hold the key
    auto nodes = findNodes(key);
    int txId = transaction.txId;
    for (int i = 0; i < nodes.size(); i++) {
        auto node = nodes[i];
        Message msg(txId, memberNode->addr, CREATE, key, value);
        msg.replica = static_cast<ReplicaType>(i);
        sendMessage(node.nodeAddress, msg);
    }
}

/**
* FUNCTION NAME: clientRead
*
* DESCRIPTION: client side READ API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientRead(string key){
    // start a transaction
    Transaction transaction(READ);
    txMap.emplace(transaction.txId, transaction);
    transaction.key = key;

    // send messages to all nodes who should hold the key
    auto nodes = findNodes(key);
    int txId = transaction.txId;
    for (int i = 0; i < nodes.size(); i++) {
        auto node = nodes[i];
        Message msg(txId, memberNode->addr, READ, key);
        msg.replica = static_cast<ReplicaType>(i);
        sendMessage(node.nodeAddress, msg);
    }
}

/**
* FUNCTION NAME: clientUpdate
*
* DESCRIPTION: client side UPDATE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientUpdate(string key, string value){
    // start a transaction
    Transaction transaction(UPDATE);
    txMap.emplace(transaction.txId, transaction);
    transaction.key = key;
    transaction.value = value;

    // send messages to all nodes who should hold the key
    auto nodes = findNodes(key);
    int txId = transaction.txId;
    for (int i = 0; i < nodes.size(); i++) {
        auto node = nodes[i];
        Message msg(txId, memberNode->addr, UPDATE, key, value);
        msg.replica = static_cast<ReplicaType>(i);
        sendMessage(node.nodeAddress, msg);
    }
}

/**
* FUNCTION NAME: clientDelete
*
* DESCRIPTION: client side DELETE API
*                 The function does the following:
*                 1) Constructs the message
*                 2) Finds the replicas of this key
*                 3) Sends a message to the replica
*/
void MP2Node::clientDelete(string key){
    // start a transaction
    Transaction transaction(DELETE);
    txMap.emplace(transaction.txId, transaction);
    transaction.key = key;

    // send messages to all nodes who should hold the key
    auto nodes = findNodes(key);
    int txId = transaction.txId;
    for (int i = 0; i < nodes.size(); i++) {
        auto node = nodes[i];
        Message msg(txId, memberNode->addr, DELETE, key);
        msg.replica = static_cast<ReplicaType>(i);
        sendMessage(node.nodeAddress, msg);
    }
}

/**
* FUNCTION NAME: createKeyValue
*
* DESCRIPTION: Server side CREATE API
*                    The function does the following:
*                    1) Inserts key value into the local hash table
*                    2) Return true or false based on success or failure
*/
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	// Insert key, value, replicaType into the hash table
	Entry entry(value, this->par->getcurrtime(), replica);
	auto res = ht->create(key, entry.convertToString());
	return res;
}

/**
* FUNCTION NAME: readKey
*
* DESCRIPTION: Server side READ API
*                 This function does the following:
*                 1) Read key from local hash table
*                 2) Return value
*/
string MP2Node::readKey(string key) {
	// Read key from local hash table and return value
	auto val = ht->read(key);
	if (val.empty()) {
	    return val;
	}
	Entry entry(val);
	return entry.value;
}

/**
* FUNCTION NAME: updateKeyValue
*
* DESCRIPTION: Server side UPDATE API
*                 This function does the following:
*                 1) Update the key to the new value in the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	// Update key in local hash table and return true or false
    Entry entry(value, par->getcurrtime(), replica);
    return ht->update(key, entry.convertToString());
}

/**
* FUNCTION NAME: deleteKey
*
* DESCRIPTION: Server side DELETE API
*                 This function does the following:
*                 1) Delete the key from the local hash table
*                 2) Return true or false based on success or failure
*/
bool MP2Node::deletekey(string key) {
	// Delete the key from the local hash table
	return ht->deleteKey(key);
}

/**
* FUNCTION NAME: checkMessages
*
* DESCRIPTION: This function is the message handler of this node.
*                 This function does the following:
*                 1) Pops messages from the queue
*                 2) Handles the messages according to message types
*/
void MP2Node::checkMessages() {
	char * data;
	int size;

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
	    // Pop a message from the queue
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string strMsg(data, data + size);
		Message msg(strMsg);
		// Handle the message types here
        switch (msg.type) {
            case CREATE: handleCreateMessage(msg);
            case UPDATE: handleUpdateMessage(msg);
            case READ: handleReadMessage(msg);
            case DELETE: handleDeleteMessage(msg);
            case REPLY: case READREPLY:
                handleReplyMessage(msg);
//            case : handleReadReplyMessage(msg);
        }
	}

	/*
	* This function should also ensure all READ and UPDATE operation
	* get QUORUM replies
	*/
}

/**
* FUNCTION NAME: findNodes
*
* DESCRIPTION: Find the replicas of the given keyfunction
*                 This function is responsible for finding the replicas of a key
*/
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i < (int)ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
	if ( memberNode->bFailed ) {
		return false;
	}
	else {
		return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
	}
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: stabilizationProtocol
*
* DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
*                 It ensures that there always 3 copies of all keys in the DHT at all times
*                 The function does the following:
*                1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
*                Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring.
 *
 *                This function gets run
 *
*/
void MP2Node::stabilizationProtocol() {

}


// my functions
void MP2Node::sendMessage(Address toAddr, Message msg) {
    emulNet->ENsend(&memberNode->addr, &toAddr, msg.toString());
}

void MP2Node::handleCreateMessage(Message msg) {
    Message reply(msg.transID, memberNode->addr, REPLY, msg.key, msg.value, msg.replica);
    if (!createKeyValue(msg.key, msg.value, msg.replica)) {
        log->logCreateFail(&msg.fromAddr, false, msg.transID, msg.key, msg.value);
        reply.success = false;
    } else {
        log->logCreateSuccess(&msg.fromAddr, false, msg.transID, msg.key, msg.value);
        reply.success = true;
    }
    sendMessage(msg.fromAddr, reply);
}

void MP2Node::handleUpdateMessage(Message msg) {
    Message reply(msg.transID, memberNode->addr, REPLY, msg.key, msg.value, msg.replica);
    if (!updateKeyValue(msg.key, msg.value, msg.replica)) {
        log->logUpdateFail(&msg.fromAddr, false, msg.transID, msg.key, msg.value);
        reply.success = false;
    } else {
        log->logUpdateSuccess(&msg.fromAddr, false, msg.transID, msg.key, msg.value);
        reply.success = true;
    }
    sendMessage(msg.fromAddr, reply);
}

void MP2Node::handleReadMessage(Message msg) {
    string res = readKey(msg.key);
    Message reply(msg.transID, memberNode->addr, READREPLY, msg.key, msg.value, msg.replica);
    if (res.empty()) {
        log->logReadFail(&msg.fromAddr, false, msg.transID, msg.key);
        reply.success = false;
    } else {
        log->logReadSuccess(&msg.fromAddr, false, msg.transID, msg.key, res);
        reply.success = true;
    }
    sendMessage(msg.fromAddr, reply);
}

void MP2Node::handleDeleteMessage(Message msg) {
    Message reply(msg.transID, memberNode->addr, REPLY, msg.key, msg.value, msg.replica);
    if (!deletekey(msg.key)) {
        log->logDeleteFail(&msg.fromAddr, false, msg.transID, msg.key);
        reply.success = false;
    } else {
        log->logDeleteSuccess(&msg.fromAddr, false, msg.transID, msg.key);
        reply.success = true;
    }
    sendMessage(msg.fromAddr, reply);
}

void MP2Node::handleReplyMessage(Message msg) {
    int txId = msg.transID;
    auto it = txMap.find(txId);
    if (it != txMap.end()) {
        Transaction *transaction = &it->second;

        transaction->totalCount++;
        if (msg.success) {
            transaction->successCount++;
        }
        if (transaction->successCount >= QUORUM) { // operation successful! log success as coordinator
            txMap.erase(txId);
            switch (transaction->type) {
                case READ: log->logReadSuccess(&memberNode->addr, true, txId, transaction->key, transaction->value);
                case UPDATE: log->logUpdateSuccess(&memberNode->addr, true, txId, transaction->key, transaction->value);
                case CREATE: log->logCreateSuccess(&memberNode->addr, true, txId, transaction->key, transaction->value);
                case DELETE: log->logDeleteSuccess(&memberNode->addr, true, txId, transaction->key);
            }
        } else if (transaction->successCount < QUORUM && transaction->totalCount == TOTAL) { // operation failed :( log failure as coordinator
            txMap.erase(txId);
            switch (transaction->type) {
                case READ: log->logReadFail(&memberNode->addr, true, txId, transaction->key);
                case UPDATE: log->logUpdateFail(&memberNode->addr, true, txId, transaction->key, transaction->value);
                case CREATE: log->logCreateFail(&memberNode->addr, true, txId, transaction->key, transaction->value);
                case DELETE: log->logDeleteFail(&memberNode->addr, true, txId, transaction->key);
            }
        }
    }
    // otherwise, we don't do anything, since if transaction doesn't exist in the map, it's already resolved
}

void MP2Node::handleReadReplyMessage(Message msg) {

}

Transaction::Transaction(MessageType _type) {
    this->successCount = 0;
    this->totalCount = 0;
    this->type = _type;
    this->txId = g_transID++;
}