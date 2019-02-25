#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE 1
#define FALSE 0 

/* allow xml element key word */
#define XML_OBJ_ELEMENT         "object"		// Element mapping to TR69 object node
#define XML_PARAM_ELEMENT       "parameter"		// Element mapping to TR69 parameter of specific object
#define XML_DESC_ELEMENT        "description"		// Element use to decript another element
#define XML_VALID_STR_ARRAY	"validstringarray"	// Element use to restrict string value for string type parameter
#define XML_VALID_STR_ELEMENT	"element"		// Element which is string value in XML_VALID_STR_ARRAY

/* allow common attribute */
#define XML_COMMON_ATTR_NAME		"name"
#define XML_COMMON_ATTR_SPEC		"specSource"
#define XML_COMMON_ATTR_PROFILE		"profile"	/* the element is belong to this profile */

/* allow object attribute */
#define XML_OBJ_ATTR_SHORT_OBJ_NAME	"shortObjectName"
#define XML_OBJ_ATTR_OBJ_TYPE		"supportLevel"		/* value should be "Present", "MultipleInstances" */

/* allow parameter attribut */
#define XML_PARAM_ATTR_TYPE		"type"			/* boolean, int, unsignedInt, string ... */
#define XML_PARAM_ATTR_PERMISSIN	"supportLevel"		/* value is belong to "ReadOnly", "ReadWrite" */
#define XML_PARAM_ATTR_STR_MAX_LEN	"maxLength"		/* Max string length of string type parameter */
#define XML_PARAM_ATTR_VALID_STR	XML_VALID_STR_ARRAY	/* Valid string of string type parameter */

#define XML_PARAM_ATTR_PERMOSION_R_ONLY "ReadOnly"
#define XML_PARAM_ATTR_PERMOSION_RW	"ReadWrite"

/* translate to cwmp of Realtek SDK */
#define ROOT_OBJ_NAME	"Root"
#define H_FILE_NAME	"prmt_%s.h"
#define C_FILE_NAME	"prmt_%s.c"

// folloing is the naming rule for CWMP
#define CWMP_OBJ_NAME		"%sObj"		//fill in "short obj name"
#define CWMP_LEAF_NAME          CWMP_OBJ_NAME"Leaf"     //fill in "short obj name"
#define CWMP_PRMT_INFO_NAME	CWMP_OBJ_NAME"Info"	//fill in "short obj name"
#define CWMP_LEAF_INFO_NAME	CWMP_LEAF_NAME"Info"	//fill in "short obj name"
#define CWMP_OBJ_OP_NAME        CWMP_OBJ_NAME"OP"       //fill in "short obj name"
#define CWMP_LEAF_OP_NAME	CWMP_LEAF_NAME"OP"       //fill in "short obj name"
#define CWMP_OBJ_NODE_NAME	CWMP_OBJ_NAME
#define CWMP_LEAF_NODE_NAME	CWMP_LEAF_NAME

#define CWMP_OBJ_OP_TABLE_ENTRY	"struct CWMP_OP "CWMP_OBJ_OP_NAME" = { %s, %s };\n"	//fill in "short obj name" + genFullOP_FuncName + genFullOP_FuncName
#define CWMP_LEAF_OP_TABLE_ENTRY "struct CWMP_OP "CWMP_LEAF_OP_NAME" = { %s, %s };\n"        //fill in "short obj name" + genFullOP_FuncName + genFullOP_FuncName
#define CWMP_GET_PROTOTYPE	"int %s(char *name, struct CWMP_LEAF *entity, int *type, void **data);\n"
#define CWMP_SET_PROTOTYPE	"int %s(char *name, struct CWMP_LEAF *entity, int type, void *data);\n"

#define CWMP_OBJ_INFO_BEGIN	"struct CWMP_PRMT "CWMP_PRMT_INFO_NAME"[] ={\n"		// fill in "short obj name"
#define CWMP_OBJ_INFO_ENTRY     "{\"%s\",    eCWMP_tOBJECT,    CWMP_READ,    &"CWMP_OBJ_OP_NAME"}"	// Object is read only
#define CWMP_OBJ_INFO_END 	"\n};\n"
#define CWMP_OBJ_INFO_ENUM_BEGIN	"enum e"CWMP_PRMT_INFO_NAME" {\n"	// fill in "short obj name"
#define CWMP_OBJ_INFO_ENUM_ENTRY	"e%s"         // fill in "short obj name"
#define CWMP_OBJ_INFO_ENUM_END		"\n};\n"

#define CWMP_LEAF_INFO_BEGIN     "struct CWMP_PRMT "CWMP_LEAF_INFO_NAME"[] ={\n"	// fill in "short obj name"
#define CWMP_LEAF_INFO_ENTRY     "{\"%s\",    %s,    %s,    &"CWMP_LEAF_OP_NAME"}"
#define CWMP_LEAF_INFO_END       "\n};\n"
#define CWMP_LEAF_INFO_ENUM_BEGIN	"enum e"CWMP_LEAF_INFO_NAME" {\n"       // fill in "short obj name"
#define CWMP_LEAF_INFO_ENUM_ENTRY	"e%s_%s"       // fill in "short obj name" + "param name"
#define CWMP_LEAF_INFO_ENUM_END		"\n};\n"

#define CWMP_OBJ_NODE_BEGIN	"struct CWMP_NODE "CWMP_OBJ_NODE_NAME"[] ={\n"	// fill in "short obj name"
#define CWMP_OBJ_NODE_ENTRY	"{&"CWMP_PRMT_INFO_NAME"["CWMP_OBJ_INFO_ENUM_ENTRY"],    %s,    %s}"		// fill in "short obj name" + {"short obj name" of child object} + {"short obj name" of child object} + {"short obj name" of child object}
#define CWMP_OBJ_NODE_END	"{NULL,    NULL,    NULL}\n};\n"
#define CWMP_LEAF_NODE_BEGIN	"struct CWMP_LEAF "CWMP_LEAF_NODE_NAME"[] ={\n"  // fill in "short obj name"
#define CWMP_LEAF_NODE_ENTRY	"{ &"CWMP_LEAF_INFO_NAME"["CWMP_LEAF_INFO_ENUM_ENTRY"] }"
#define CWMP_LEAF_NODE_END	"{ NULL }\n};\n"



enum {
  SPEC_TR098 = 0,
  SPEC_TR181
};

#define TR69_SPEC	SPEC_TR181

typedef enum {
  TYPE_OBJ = 0,
  TYPE_LEAF = 1
} XML_TYPE;

typedef enum {
  RET_OK = 0,
  RET_FAIL = 1
} RET_TYPE;

enum {
  COMMON_NAME=0,
  COMMON_SPEC,
  COMMON_PROFILE,
  COMMON_MAX	// last entry
};

enum {
  PARAM_TYPE=0,
  PARAM_PERMISSION,
  PARAM_STR_MAN_LEN,
  PARAM_VALID_STR,
  PARAM_MAX     //last entry
};

enum {
  OBJ_SHORT_NAME=0,
  OBJ_TYPE,
  OBJ_MAX
};

struct param_entry{
  char *common_attr[COMMON_MAX];
  char *attr[PARAM_MAX];
  struct param_entry *prev;
  struct param_entry *next;
};

struct obj_entry{
  unsigned int depth;
  char *common_attr[COMMON_MAX];
  char *attr[OBJ_MAX];
  struct param_entry *param;	// parameters of this object
  struct obj_entry *parent;
  struct obj_entry *child;
  struct obj_entry *prev;
  struct obj_entry *next;
};

struct attr_table_entry{
  unsigned idx;			// attribute index
  char attr_name[32];		// attribute name
};

struct translate_entry{
  char element[32];		// xml element name
  void (*startHandler)(void *userData, const char *name, const char **attr);	// translate atts to c struct in start phase
  void (*endHandler)(void *userData, const char *name);				// translate atts to c struct in end phase
};

/******************* debug function **************************/
void dump_obj(const struct obj_entry *target);
void dump_all_Object(struct obj_entry *initialObj);

/******************* init root function **************************/
void insert_rootObj(void);

/****************** xml parser function *********************/
void startHandler_obj(void *userData, const char *name, const char **attr);
void endHandler_obj(void *userData, const char *name);
void startHandler_param(void *userData, const char *name, const char **attr);
void endHandler_param(void *userData, const char *name);
void startHandler_desc(void *userData, const char *name, const char **attr);
void endHandler_desc(void *userData, const char *name);

/******************** translate api ******************/
void translate_ObjInfo(struct obj_entry *obj);
void translate_all_Object(struct obj_entry *initialObj);
