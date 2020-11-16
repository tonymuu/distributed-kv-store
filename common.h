/**********************************
 * FILE NAME: common.h
 *
 * DESCRIPTION: Common header for message / replica types (Revised 2020)
 **********************************/

#ifndef COMMON_H_
#define COMMON_H_

/**
 * Global variable
 */
// Transaction Id
static int g_transID = 0;

// Prevents erroneous warnings about variable not being used
namespace {
	struct Foo {
		Foo() {
			(void)g_transID;
		}
	};
}

// message types, reply is the message from node to coordinator
enum MessageType {CREATE, READ, UPDATE, DELETE, REPLY, READREPLY};
// enum of replica types
enum ReplicaType {PRIMARY, SECONDARY, TERTIARY};

#endif
