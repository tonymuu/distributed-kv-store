/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions. (Revised 2020)
 *
 *  Starter code template
 **********************************/

#include "MP1Node.h"
#include <csignal>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

unordered_map<int, long> failed;

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node( Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = new Member;
    this->shouldDeleteMember = true;
	memberNode->inited = false;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member* member, Params *params, EmulNet *emul, Log *log, Address *address) {
    for( int i = 0; i < 6; i++ ) {
        NULLADDR[i] = 0;
    }
    this->memberNode = member;
    this->shouldDeleteMember = false;
    this->emulNet = emul;
    this->log = log;
    this->par = params;
    this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {
    if (shouldDeleteMember) {
        delete this->memberNode;
    }
}

/**
* FUNCTION NAME: recvLoop
*
* DESCRIPTION: This function receives message from the network and pushes into the queue
*                 This function is called by a node to receive messages currently waiting for it
*/
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
* FUNCTION NAME: nodeStart
*
* DESCRIPTION: This function bootstraps the node
*                 All initializations routines for a member.
*                 Called by the application layer.
*/
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
    /*
    * This function is partially implemented and may require changes
    */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	// node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
	initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == strcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
        // add myself to my memberlist
        int id = 0;
        short port;
        setIdAndPortFromAddress(memberNode->addr, &id, &port);
        MemberListEntry entry = MemberListEntry(id, port, 0, par->getcurrtime());
        memberNode->memberList.push_back(entry);

        // log to debug log about new node join
        log->logNodeAdd(&memberNode->addr, &memberNode->addr);
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
* FUNCTION NAME: finishUpThisNode
*
* DESCRIPTION: Wind up this node and clean up state
*/
int MP1Node::finishUpThisNode(){
    memberNode->inited = false;
    memberNode->inGroup = false;
    memberNode->nnb = 0;
    memberNode->heartbeat = 0;
    memberNode->memberList.clear();
    return 0;
}

/**
* FUNCTION NAME: nodeLoop
*
* DESCRIPTION: Executed periodically at each member
*                 Check your messages in queue and perform membership protocol duties
*/
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
    // Get msg header from msg
    MessageHdr header;
    memcpy(&header, data, sizeof(MessageHdr));

    // Perform different functions based on different msg type
    if (header.msgType == JOINREQ) {
        // deserialize msg from memory
        Address addr;
        addr.init();
        long heartbeat = 0;
        memcpy(&addr.addr, data + sizeof(MessageHdr), sizeof(addr.addr));
        memcpy(&heartbeat, data + sizeof(MessageHdr) + sizeof(addr.addr), sizeof(long));

        // add to my memberlist
        int id = 0;
        short port;
        setIdAndPortFromAddress(addr, &id, &port);
        MemberListEntry entry = MemberListEntry(id, port, heartbeat, par->getcurrtime());
        memberNode->memberList.push_back(entry);

        // log to debug log about new node join
        log->logNodeAdd(&memberNode->addr, &addr);

        // send joinrep msg that includes my memberlist
        sendJoinRepMsg(&addr);
    } else if (header.msgType == JOINREP) {
        // Since I received JOINREP, I've made myself known so I'm in the group
        memberNode->inGroup = true;
        // We just joined, so we should use introducer's membership list and clear anything previously stored.
        memberNode->memberList.clear();

        // Now we deserialize the msg and update my member list
        deserializeAndUpdateMemberList(data, size);
    } else if (header.msgType == UPDATEREQ) {
        // update membership list, add any new nodes, log
        deserializeAndUpdateMemberList(data, size);
    }
}

void MP1Node::sendJoinRepMsg(Address *toAddr) {
    // Create the msg that includes header and my member list entires
    int mleSize = sizeof(int) + sizeof(short) + sizeof(long) + sizeof(long), numEntries = memberNode->memberList.size();
    size_t msgsize = sizeof(MessageHdr) + mleSize * numEntries;

    MessageHdr *msg;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = JOINREP;

    serializeMemberList(msg);
    // Reply with JOINREP message
    emulNet->ENsend(&memberNode->addr, toAddr, (char *)msg, msgsize);

    free(msg);
}

/**
* FUNCTION NAME: nodeLoopOps
*
* DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
*                 the nodes
*                 Propagate your membership list
*/
void MP1Node::nodeLoopOps() {
    long currTime = par->getcurrtime();
    auto shouldSendGossip = (currTime - timeLastGossip) >= TGOSSIP && memberNode->memberList.size() > 0;

    // select a random neighbour and send gossip
    if (shouldSendGossip) {
        memberNode->heartbeat++;
        timeLastGossip = currTime;
        int index = rand() % memberNode->memberList.size();
        auto neighbour = memberNode->memberList[index];
        Address neighbourAddr = createAddressFromIdAndPort(neighbour.id, neighbour.port);
        sendGossip(&neighbourAddr);
    }

    int id = 0;
    short port = 0;
    setIdAndPortFromAddress(memberNode->addr, &id, &port);

    // update any node that has failed, remove any failed nodes that are passed Tcleanup
    // If a node's heartbeat hasn't increased for more than Tfail (currentTime - node.timestamp), we mark it as failed
    // if a node is marked as failed for more than Tcleanup, remove the node from list, and log
    auto neighbour = memberNode->memberList.begin();
    while (neighbour != memberNode->memberList.end()) {
        Address neighbourAddr = createAddressFromIdAndPort(neighbour->id, neighbour->port);
        if (areAddressesEqual(memberNode->addr, neighbourAddr)) {
            neighbour++;
            continue;
        }

        auto hasFailed = failed.count(neighbour->id) != 0;
        long timeSinceLastUpdate = currTime - neighbour->timestamp;
        if (hasFailed && currTime - failed[neighbour->id] >= TREMOVE) { // remove node
            neighbour = memberNode->memberList.erase(neighbour);
            failed.erase(neighbour->id);
            log->logNodeRemove(&(memberNode->addr), &neighbourAddr);
            continue;
        }

        if (!hasFailed && timeSinceLastUpdate >= TFAIL) { // if node has failed, we don't send msg to it
            failed[neighbour->id] = currTime;
            hasFailed = true;
        }
        neighbour++;
    }

    return;
}

void MP1Node::sendGossip(Address *toAddr) {
    int mleSize = sizeof(int) + sizeof(short) + sizeof(long) + sizeof(long), numEntries = memberNode->memberList.size();
    size_t msgsize = sizeof(MessageHdr) + mleSize * numEntries;

    MessageHdr *msg;
    msg = (MessageHdr *) malloc(msgsize * sizeof(char));
    msg->msgType = UPDATEREQ;

    serializeMemberList(msg);

    // Reply with UPDATEREQ message
    emulNet->ENsend(&memberNode->addr, toAddr, (char *)msg, msgsize);

    free(msg);
}

void MP1Node::serializeMemberList(MessageHdr* msg) {
    int offset = 0;
    int id = 0;
    short port = 0;
    setIdAndPortFromAddress(memberNode->addr, &id, &port);
    long currTime = par->getcurrtime();
    for(auto mle = memberNode->memberList.begin(); mle != memberNode->memberList.end(); ++mle) {
        memcpy((char *)(msg+1) + offset, &mle->id, sizeof(int));
        offset += sizeof(int);

        memcpy((char *)(msg+1) + offset, &mle->port, sizeof(short));
        offset += sizeof(short);

        memcpy((char *)(msg+1) + offset, mle->id == id ? &(memberNode->heartbeat) : &(mle->heartbeat), sizeof(long));
        offset += sizeof(long);

        memcpy((char *)(msg+1) + offset, mle->id == id ? &currTime : &(mle->timestamp), sizeof(long));
        offset += sizeof(long);
    }
}

void MP1Node::deserializeAndUpdateMemberList(char *data, int size) {
    // map of port, heartbeat, timestamp
    auto map = createMemberlistMap();

    auto currTime = par->getcurrtime();

    int mleSize = sizeof(int) + sizeof(short) + sizeof(long) + sizeof(long);
    int numEntries = (size - sizeof(MessageHdr)) / mleSize;
    int offset = sizeof(MessageHdr);

    for (int i = 0; i < numEntries; i++) {
        // get the member list
        int id = 0;
        short port = 0;
        long heartbeat = 0;
        long timestamp = 0;

        memcpy(&id, data + offset, sizeof(int));
        offset += sizeof(int);

        memcpy(&port, data + offset, sizeof(short));
        offset += sizeof(short);

        memcpy(&heartbeat, data + offset, sizeof(long));
        offset += sizeof(long);

        memcpy(&timestamp, data + offset, sizeof(long));
        offset += sizeof(long);

        // if member doesn't exist in current member list, we add it there
//        MemberListEntry* old = getNodeFromMemberListTable(id);
        if (map.count(id) == 0) {
//        if (old == nullptr) {
            MemberListEntry entry = MemberListEntry(id, port, heartbeat, currTime);
            memberNode->memberList.push_back(entry);

            // log the new mle join
            Address addr = createAddressFromIdAndPort(id, port);
            log->logNodeAdd(&memberNode->addr, &addr);
        } else {
            // update the existing member entry
            auto old = map[id];
            if (old->heartbeat < heartbeat) {
                if (failed.count(id) > 0) {
                    failed.erase(id);
                }
                old->setheartbeat(heartbeat);
                old->settimestamp(currTime);
            }
        }
    }
}


/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}

void MP1Node::setIdAndPortFromAddress(Address addr, int *id, short *port) {
    memcpy(id, &addr.addr, sizeof(int));
    memcpy(port, &addr.addr[4], sizeof(short));
}

Address MP1Node::createAddressFromIdAndPort(int id, short port) {
    Address addr;
    addr.init();
    *(int *)(&(addr.addr))=id;
    *(short *)(&(addr.addr[4]))=port;
    return addr;
}

bool MP1Node::areAddressesEqual(Address addr1, Address addr2) {
    return memcmp((char*)&addr1, (char*)&addr2, sizeof(Address)) == 0;
}

map<int, MemberListEntry*> MP1Node::createMemberlistMap() {
    map<int, MemberListEntry*> map;
    for (auto neighbour = memberNode->memberList.begin(); neighbour != memberNode->memberList.end(); neighbour++) {
        map[neighbour->id] = &*neighbour;
    }
    return map;
}

MemberListEntry* MP1Node::getNodeFromMemberListTable(int id) {
    for(auto mle = memberNode->memberList.begin(); mle != memberNode->memberList.end(); ++mle) {
        if(mle->id == id) {
            return mle.base();
        }
    }
    return nullptr;
}