#include "open62541/include/open62541.h"
#include <signal.h>
#include <stdio.h>
#include <snap7.h>
#include "s7.h"
#include <sstream>
#include <iostream>
#include <cctype>
#include <vector>
#include "minIni.h"

#define sizearray(a) (sizeof(a) / sizeof((a)[0]))

using namespace std;

byte Buffer[10548];
TS7Client *Client;
UA_Boolean running = true;
const char inifile[] = "conf.ini";

static void stopHandler(int sig) {
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c"); // @suppress("Invalid arguments")
	running = false;
}

static char* str2char(string str) {
	char *a = new char[str.size() + 1];
	a[str.size()] = 0;
	memcpy(a, str.c_str(), str.size()); // @suppress("Invalid arguments")
	return a;
}

static UA_StatusCode writeCurrentInt(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext,
		const UA_NumericRange *range, const UA_DataValue *data) {
	UA_Variant val = data->value;
	int pos = *(int *) nodeContext;
	UA_Int32 value = *(UA_Int32*) val.data;
	S7_SetDIntAt(Buffer, pos, (long) value);
	Client->DBWrite(2, 0, 10544, Buffer);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"The variable at pos %i was updated with value %i", pos, value);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readCurrentInt(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
		const UA_NumericRange *range, UA_DataValue *dataValue) {
	Client->DBRead(2, 0, 10544, &Buffer);
	int pos = *(int *) nodeContext;
	long temp = S7_GetDIntAt(Buffer, pos);
	UA_Int32 val = (UA_Int32) temp;
	UA_Variant_setScalarCopy(&dataValue->value, &val,
			&UA_TYPES[UA_TYPES_INT32]);
	dataValue->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

static void addIntVariable(UA_Server *server, char* name, int pos) {
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
	attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

	UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
	UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0,
	UA_NS0ID_BASEDATAVARIABLETYPE);

	int* tmp = new int(pos);
	UA_DataSource intDataSource;
	intDataSource.read = readCurrentInt;
	intDataSource.write = writeCurrentInt;
	UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
			parentReferenceNodeId, currentName, variableTypeNodeId, attr,
			intDataSource, tmp, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
				"Added %s variable.", name);
}

static UA_StatusCode writeCurrentBool(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext,
		const UA_NumericRange *range, const UA_DataValue *data) {
	UA_Variant val = data->value;
	int pos = *(int *) nodeContext;
	UA_Boolean value = *(UA_Boolean*) val.data;
	S7_SetBitAt(Buffer, pos, 0, (bool) value);
	Client->DBWrite(2, 0, 10544, Buffer);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"The variable at pos %i was updated with value %b", pos, value);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readCurrentBool(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
		const UA_NumericRange *range, UA_DataValue *dataValue) {
	Client->DBRead(2, 0, 10544, &Buffer);
	int pos = *(int *) nodeContext;
	bool temp = S7_GetBitAt(Buffer, pos, 0);
	UA_Boolean val = (UA_Boolean) temp;
	UA_Variant_setScalarCopy(&dataValue->value, &val,
			&UA_TYPES[UA_TYPES_BOOLEAN]);
	dataValue->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

static void addBoolVariable(UA_Server *server, char* name, int pos) {
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
	attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

	UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
	UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0,
	UA_NS0ID_BASEDATAVARIABLETYPE);

	int* tmp = new int(pos);
	UA_DataSource boolDataSource;
	boolDataSource.read = readCurrentBool;
	boolDataSource.write = writeCurrentBool;
	UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
			parentReferenceNodeId, currentName, variableTypeNodeId, attr,
			boolDataSource, tmp, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
				"Added %s variable.", name);
}

static UA_StatusCode writeCurrentString(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext,
		const UA_NumericRange *range, const UA_DataValue *data) {
	UA_Variant val = data->value;
	int pos = *(int *) nodeContext;
	UA_String value = *(UA_String*) val.data;
	char* outstring = (char*) UA_malloc(sizeof(char) * value.length + 1); // @suppress("Invalid arguments")
	memcpy(outstring, value.data, value.length); // @suppress("Invalid arguments")
	outstring[value.length] = '\0';
	S7_SetStringAt(Buffer, pos, 15, outstring);
	Client->DBWrite(2, 0, 10544, Buffer);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"The variable at pos %i was updated with value: ", pos, value);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readCurrentString(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
		const UA_NumericRange *range, UA_DataValue *dataValue) {
	Client->DBRead(2, 0, 10544, &Buffer);
	int pos = *(int *) nodeContext;
	string temp = S7_GetStringAt(Buffer, pos);

	UA_String val = UA_STRING(str2char(temp));
	UA_Variant_setScalarCopy(&dataValue->value, &val,
			&UA_TYPES[UA_TYPES_STRING]);
	dataValue->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

static void addStringVariable(UA_Server *server, char* name, int pos) {
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
	attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

	UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
	UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0,
	UA_NS0ID_BASEDATAVARIABLETYPE);

	int* tmp = new int(pos);
	UA_DataSource stringDataSource;
	stringDataSource.read = readCurrentString;
	stringDataSource.write = writeCurrentString;
	UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
			parentReferenceNodeId, currentName, variableTypeNodeId, attr,
			stringDataSource, tmp, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"Added %s variable.", name);
}

static UA_StatusCode writeCurrentTime(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext,
		const UA_NumericRange *range, const UA_DataValue *data) {
	UA_Variant val = data->value;
	int pos = *(int *) nodeContext;
	UA_String value = *(UA_String*) val.data;
	char* outstring = (char*) UA_malloc(sizeof(char) * value.length + 1); // @suppress("Invalid arguments")
	memcpy(outstring, value.data, value.length); // @suppress("Invalid arguments")
	outstring[value.length] = '\0';
	int y, M, d, h, m, s;
	sscanf(outstring, "%d-%d-%dT%d:%d:%d", &y, &M, &d, &h, &m, &s);
	S7_SetUIntAt(Buffer, pos, y);
	S7_SetByteAt(Buffer, pos + 2, M);
	S7_SetByteAt(Buffer, pos + 3, d);
	S7_SetByteAt(Buffer, pos + 5, h);
	S7_SetByteAt(Buffer, pos + 6, m);
	S7_SetByteAt(Buffer, pos + 7, s);
	Client->DBWrite(2, 0, 10544, Buffer);
	UA_LOG_INFO(UA_Log_Stdout,
			UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"The variable at pos %i was updated with date:  %d-%d-%dT%d:%d:%d",
			pos, y, M, d, h, m, s);
	return UA_STATUSCODE_GOOD;
}

static UA_StatusCode readCurrentTime(UA_Server *server,
		const UA_NodeId *sessionId, void *sessionContext,
		const UA_NodeId *nodeId, void *nodeContext, UA_Boolean sourceTimeStamp,
		const UA_NumericRange *range, UA_DataValue *dataValue) {
	Client->DBRead(2, 0, 10544, &Buffer);
	int pos = *(int *) nodeContext;
	int y, M, d, h, m, s;
	y = S7_GetUIntAt(Buffer, pos);
	M = S7_GetByteAt(Buffer, pos + 2);
	d = S7_GetByteAt(Buffer, pos + 3);
	h = S7_GetByteAt(Buffer, pos + 5);
	m = S7_GetByteAt(Buffer, pos + 6);
	s = S7_GetByteAt(Buffer, pos + 7);
	char temp[18];
	sprintf(temp, "%d-%d-%dT%d:%d:%d", y, M, d, h, m, s);
	UA_String val = UA_STRING(temp);
	UA_Variant_setScalarCopy(&dataValue->value, &val,
			&UA_TYPES[UA_TYPES_STRING]);
	dataValue->hasValue = true;
	return UA_STATUSCODE_GOOD;
}

static void addTimeVariable(UA_Server *server, char* name, int pos) {
	UA_VariableAttributes attr = UA_VariableAttributes_default;
	attr.displayName = UA_LOCALIZEDTEXT("en-US", name);
	attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;

	UA_NodeId currentNodeId = UA_NODEID_STRING(1, name);
	UA_QualifiedName currentName = UA_QUALIFIEDNAME(1, name);
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableTypeNodeId = UA_NODEID_NUMERIC(0,
	UA_NS0ID_BASEDATAVARIABLETYPE);

	int* tmp = new int(pos);
	UA_DataSource timeDataSource;
	timeDataSource.read = readCurrentTime;
	timeDataSource.write = writeCurrentTime;
	UA_Server_addDataSourceVariableNode(server, currentNodeId, parentNodeId,
			parentReferenceNodeId, currentName, variableTypeNodeId, attr,
			timeDataSource, tmp, NULL);
	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"Added %s variable.", name);
}

int main(void) {
	char str[100];
	long n;
	int s, k, pos;
	char section[50];
	char type[10];
	char name[128];

	signal(SIGINT, stopHandler);
	signal(SIGTERM, stopHandler);

	UA_ServerConfig *config = UA_ServerConfig_new_default();
	UA_Server *server = UA_Server_new(config);
	Client = new TS7Client();
	Client->ConnectTo("192.168.11.32", 0, 1);
	for (s = 0; ini_getsection(s, section, sizearray(section), inifile) > 0;
			s++) {
		for (k = 0; ini_getkey(section, k, str, sizearray(str), inifile) > 0;
				k++) {
			switch (k) {
			case 0:
				ini_gets(section, str, "nan", name, 128, inifile);
				break;
			case 1:
				ini_gets(section, str, "nan", type, 10, inifile);
				break;
			case 2:
				pos = ini_getl(section, str, -1, inifile);
			}
		}
		if (strcmp(type, "boolean") == 0) {
			addBoolVariable(server, name, pos);
		}
		if (strcmp(type, "integer") == 0) {
			addIntVariable(server, name, pos);
		}
		if (strcmp(type, "string") == 0) {
			addStringVariable(server, name, pos);
		}
		if (strcmp(type, "timestamp") == 0) {
			addTimeVariable(server, name, pos);
		}/* for */
	}

	UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, // @suppress("Invalid arguments")
			"All variables were added.");
	UA_StatusCode retval = UA_Server_run(server, &running);
	delete Client;
	UA_Server_delete(server);
	UA_ServerConfig_delete(config);
	return (int) retval;
}

