#include "xml_translate.h"

#define MAX_BUFF_LEN 10000	// Max buff length should be tIGDObject[] of TR98
#define MAX_SHORT_NAME 64	// Max object short name length

struct obj_entry *rootObj=NULL;	// the root object for this spec
struct obj_entry *presentObj=NULL; // current Object

//xml value string
struct validstr *obj_type=NULL;
struct validstr *param_type=NULL;
struct validstr *param_perm=NULL;

struct attr_table_entry common_attr[]={
  {COMMON_NAME, XML_COMMON_ATTR_NAME},
  {COMMON_SPEC, XML_COMMON_ATTR_SPEC},
  {COMMON_PROFILE, XML_COMMON_ATTR_PROFILE}
};

struct attr_table_entry obj_attr[]={
  {OBJ_SHORT_NAME, XML_OBJ_ATTR_SHORT_OBJ_NAME},
  {OBJ_TYPE, XML_OBJ_ATTR_OBJ_TYPE}
};

struct attr_table_entry param_attr[]={
  {PARAM_TYPE, XML_PARAM_ATTR_TYPE},
  {PARAM_PERMISSION, XML_PARAM_ATTR_PERMISSIN},
  {PARAM_STR_MAN_LEN, XML_PARAM_ATTR_STR_MAX_LEN},
  {PARAM_VALID_STR, XML_PARAM_ATTR_VALID_STR}
};

struct param_type_table_entry cwmp_type[]={
  { XML_PARAM_ATTR_TYPE_STR, "eCWMP_tSTRING"},
  { XML_PARAM_ATTR_TYPE_INT, "eCWMP_tINT"},
  { XML_PARAM_ATTR_TYPE_UINT, "eCWMP_tUINT"},
  { XML_PARAM_ATTR_TYPE_BOOL, "eCWMP_tBOOLEAN"},
  { XML_PARAM_ATTR_TYPE_DATE, "eCWMP_tDATETIME"},
  { XML_PARAM_ATTR_TYPE_BASE64, "eCWMP_tBASE64"},
  { XML_PARAM_ATTR_TYPE_ULONG, "eCWMP_tULONG"},
  { XML_PARAM_ATTR_TYPE_HEXBIN, "eCWMP_tHEXBIN"}
};

struct param_type_table_entry cwmp_permission[]={
  { XML_PARAM_ATTR_PERMOSION_R_ONLY, "CWMP_READ"},
  { XML_PARAM_ATTR_PERMOSION_RW, "CWMP_READ|CWMP_WRITE"}
};

#define PARAM_TYPE_NUM  ((int)(sizeof(cwmp_type)/sizeof(struct param_type_table_entry)))
#define PARAM_PERM_NUM  ((int)(sizeof(cwmp_permission)/sizeof(struct param_type_table_entry)))


/******************* debug function **************************/
void dump_param(const unsigned int depth, const struct param_entry *target){
  int i;

  if(!target)
    return;

  for(i=0; i<depth+1; i++){
    if(i == depth)
      printf("|     ");
    else
      printf("  ");  
  }
  /* show common attribute */
  for(i=0; i<COMMON_MAX; i++){
    if(target->common_attr[common_attr[i].idx])
      printf("%s=\"%s\", ", common_attr[i].attr_name, target->common_attr[common_attr[i].idx]);
  }

  /* show parameter attribute */
  for(i=0; i<PARAM_MAX; i++){
    if(target->attr[param_attr[i].idx])
      printf("%s=\"%s\", ", param_attr[i].attr_name, target->attr[param_attr[i].idx]);
  }
  printf("\n");
  return;
}

void dump_obj(const struct obj_entry *target){
  int i;
  struct param_entry *tmp=NULL;

  if(!target)
    return;

  for(i=0; i<target->depth; i++){
    if(i == (target->depth-1))
      printf("|-");
    else
      printf("  ");
  }
  printf("[OBJ] ");
  /* show common attribute */
  for(i=0; i<COMMON_MAX; i++){
    if(target->common_attr[common_attr[i].idx])
      printf("%s=\"%s\", ", common_attr[i].attr_name, target->common_attr[common_attr[i].idx]);
  }

  /* show object attribute */
  for(i=0; i<OBJ_MAX; i++){
    if(target->attr[obj_attr[i].idx])
      printf("%s=\"%s\", ", obj_attr[i].attr_name, target->attr[obj_attr[i].idx]);
  }
  printf("\n");

  /* dump object's parameter */
  tmp = target->param;
  while(tmp){
    dump_param(target->depth, tmp);
    tmp = tmp->next;
  }
  return;
}

void dump_all_Object(struct obj_entry *initialObj){
  struct obj_entry *currObj=initialObj;
  //printf("[dump_all_Object]rootObj=%p, currObj=%p, initialObj=%p\n", rootObj, currObj, initialObj);
  while(currObj != NULL){
    dump_obj(currObj);
    if(currObj->child){
      /* dump child first */
      dump_all_Object(currObj->child);
    }

    currObj = currObj->next;
  }
  return;
}
/******************* debug function **************************/

/******************* linked list api ******************/
void resort_depth(struct obj_entry *initialObj, int isAdd){
  struct obj_entry *currObj=initialObj;
  //printf("[dump_all_Object]rootObj=%p, currObj=%p, initialObj=%p\n", rootObj, currObj, initialObj);
  while(currObj != NULL){
    if(currObj->child){
      /* dump child first */
      resort_depth(currObj->child, isAdd);
    }

    currObj->depth++;
    currObj = currObj->next;
  }
  return;
}

void insert_rootObj(void){
  struct obj_entry *root=NULL;

  /* allocate and memset to zero */
  root = (struct obj_entry *)calloc(1, sizeof(struct obj_entry));
  if(!root){
    printf("[%s]calloc fail\n", __FUNCTION__);
    return;
  }

  root->common_attr[COMMON_NAME] = (char *)calloc(1, strlen(ROOT_OBJ_NAME)+1);
  sprintf(root->common_attr[COMMON_NAME], ROOT_OBJ_NAME);

  root->attr[OBJ_SHORT_NAME] = (char *)calloc(1, strlen(ROOT_OBJ_NAME)+1);
  sprintf(root->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME);

  root->attr[OBJ_TYPE] = (char *)calloc(1, strlen(XML_OBJ_ATTR_TYPE_SINGLE)+1);
  sprintf(root->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_SINGLE);

  root->child = rootObj;
  root->parent = root;
  rootObj = root;

  resort_depth(rootObj->child, 1);

  return;
}

void insert_obj(struct obj_entry **currObj, struct obj_entry *newObj){
  if(!newObj){
    printf("[%s] NULL newObj\n", __FUNCTION__);
    return;
  }

  //printf("rootObj=%p, currObj=%p, newObj=%p\n", rootObj, *currObj, newObj);

  if(!*currObj){
    /* currObj is NULL, assign newObj to currObj */
    if(newObj->common_attr[COMMON_NAME]){
      *currObj = newObj;
      presentObj = newObj;	// assign the new to present for parameter handler
      //printf("  assigned to current\n");
    }
  }else{
    /* we assume that xml file is follow correct order */
    if(newObj->common_attr[COMMON_NAME]){
      if(!strncmp((*currObj)->common_attr[COMMON_NAME], newObj->common_attr[COMMON_NAME], strlen((*currObj)->common_attr[COMMON_NAME]))){
        /* new object is belong to currObj sub-object */
        newObj->parent = (*currObj);
        newObj->depth += 1;
        //printf("new object is belong to currObj sub-object, search child.\n");
        insert_obj((&(*currObj)->child), newObj);
        return;
      }else{
        /* new object is belong to this Depth */
        newObj->prev = *currObj;
        //printf("new object is belong to this Depth, search next.\n");
        insert_obj(&((*currObj)->next), newObj);
        return;
      }
    }else{
      printf("[%s] object name is \n", newObj->common_attr[COMMON_NAME]);
    }
  }
  return;
}

void insert_param(struct param_entry **curr, struct param_entry *new){
  if(!new){
    printf("[%s] NULL new\n", __FUNCTION__);
    return;
  }

  if(!*curr){
    /* curr is NULL, assign new to curr */
    *curr = new;
    //printf("  assigned to current\n");
  }else{
    /* assign to last one */
    new->prev = *curr;
    //printf("new object is belong to this Depth, search next.\n");
    insert_param(&((*curr)->next), new);
  }
  return;
}

void add_validstr(struct validstr **curr, const char *new){
    if(!new){
      printf("[%s] NULL new\n", __FUNCTION__);
      return;
    }
    
    if(!*curr){
      /* curr is NULL, assign new to curr */
	  *curr = (struct validstr *)calloc(1, sizeof(struct validstr));
      (*curr)->str = strdup(new);
      //printf("  assigned to current\n");
    }else{
      /* assign to last one */
      add_validstr(&((*curr)->next), new);
    }
    return;
}

void del_validstr(struct validstr *prev, struct validstr **curr, const char *new){
    if(!*curr || !new){
      printf("[%s] NULL new\n", __FUNCTION__);
      return;
    }
    
    if(!strncmp((*curr)->str, new, strlen((*curr)->str))){
      free((*curr)->str);
      if(!prev){
      	*curr = (*curr)->next;
      }else{
        prev->next = (*curr)->next;
      }

      free(*curr);
    }else{
      /* assign to last one */
      del_validstr(*curr, &((*curr)->next), new);
    }
    return;
}
/******************* linked list api ******************/
void initXml2c(void){
  // init Object XML
  add_validstr(&obj_type, XML_OBJ_ATTR_TYPE_SINGLE);
  add_validstr(&obj_type, XML_OBJ_ATTR_TYPE_MULTIPLE);

  // init parameter XML
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_STR);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_INT);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_UINT);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_BOOL);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_DATE);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_BASE64);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_ULONG);
  add_validstr(&param_type, XML_PARAM_ATTR_TYPE_HEXBIN);

  add_validstr(&param_perm, XML_PARAM_ATTR_PERMOSION_R_ONLY);
  add_validstr(&param_perm, XML_PARAM_ATTR_PERMOSION_RW);

  return;
}

int isValidStr(struct validstr *base, const char *value){
  if(!value)
    return FALSE;

  struct validstr *tmp = base;
  while(tmp){
  	if(tmp->str && !strncmp(tmp->str, value, strlen(tmp->str))){
      return TRUE;
    }
	tmp=tmp->next;
  }

  return FALSE;
}

void initCWMP_File(void){
  FILE *h_fp=NULL, *c_fp=NULL;
  char h_filename[32], c_filename[32];
  char shortName[MAX_SHORT_NAME];

  memset(shortName, 0, sizeof(MAX_SHORT_NAME));

  /* translate to one file */
  snprintf(shortName, MAX_SHORT_NAME, "%s", ROOT_OBJ_NAME);

  sprintf(h_filename, H_FILE_NAME, shortName);
  sprintf(c_filename, C_FILE_NAME, shortName);

  unlink(h_filename);
  unlink(c_filename);

  if(!((h_fp=fopen(h_filename, "a+")) && (c_fp=fopen(c_filename, "a+")))){
    if(h_fp)
      fclose(h_fp);
    if(c_fp)
      fclose(c_fp);
    return;
  }

  // write include in c file
  fprintf (c_fp, LOCAL_INCLUDE, h_filename);

  fprintf (c_fp, "\n\n");


  // write include in h file
  fprintf (h_fp, STANDARD_INCLUDE, "stdio.h");
  fprintf (h_fp, STANDARD_INCLUDE, "stdlib.h");
  fprintf (h_fp, STANDARD_INCLUDE, "string.h");
  fprintf (h_fp, LOCAL_INCLUDE, CWMP_LIB_H_FILE);
  fprintf (h_fp, LOCAL_INCLUDE, CWMP_MIB_H_FILE);

  fprintf (h_fp, "\n\n");

  if(h_fp)
    fclose(h_fp);
  if(c_fp)
    fclose(c_fp);

  return;
}

/*
 *  startHandler_obj: the handler of object in start phase
 *    input parameter:
 *      attr: attribut array of element
 *      name: attribute name
 *    output parameter:
 *      NULL
 *    return value:
 *      attribute value of name
 */
static const char *getXmlAttrByName(const char **attr, const char *name){
  int i,j;

  for (i = 0; attr[i]; i += 2) {
    if(!strncmp(name, attr[i], strlen((const char*)name))){
      // check attr value is vaild or not
      if(!strncmp(attr[i], XML_PARAM_ATTR_TYPE, strlen(XML_PARAM_ATTR_TYPE))){
        for(j=0; j<PARAM_TYPE_NUM; j++){
          if(!strncmp(cwmp_type[j].xml, attr[i+1], strlen(cwmp_type[j].xml))){
            return attr[i+1];
          }
        }
        printf("[%s] invalid attribute value(%s).\n", __FUNCTION__, attr[i+1]);
        return NULL;
#if 0
      }else if(!strncmp(attr[i], XML_PARAM_ATTR_PERMISSIN, strlen(XML_PARAM_ATTR_PERMISSIN))){
        for(j=0; j<PARAM_PERM_NUM; j++){
          if(!strncmp(cwmp_permission[j].xml, attr[i+1], strlen(cwmp_permission[j].xml))){
            return attr[i+1];
          }
        }
        printf("[%s] invalid attribute value(%s).\n", __FUNCTION__, attr[i+1]);
        return NULL;
#endif
      }else{
        /* found target */
        return attr[i+1];
      }
    }
  }

  return NULL;
}

void add_attr(char **dst,const char *src){

  if(src){
    /* allocate and memset to zero */
    *dst = (char *)calloc(1, strlen(src)+1);
    strncpy(*dst, src, strlen(src));
  }

  return;
}

/* 
 *  startHandler_obj: the handler of object in start phase
 *    input parameter:
 *      userData:
 *      name: element name
 *      attr: attribut array of element
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void startHandler_obj(void *userData, const char *name, const char **attr){
  struct obj_entry *new=NULL;
  int i;

  if(!getXmlAttrByName(attr, XML_COMMON_ATTR_NAME)){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_COMMON_ATTR_NAME);
    return;
  }

  if(!isValidStr(obj_type, getXmlAttrByName(attr, XML_OBJ_ATTR_OBJ_TYPE))){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_OBJ_ATTR_OBJ_TYPE);
    return;
  }

  /* allocate and memset to zero */
  new = (struct obj_entry *)calloc(1, sizeof(struct obj_entry));
  if(!new){
    printf("[%s]calloc fail\n", __FUNCTION__);
    return;
  }

  /* assign common attribute */
  for(i=0; i<COMMON_MAX; i++)
    add_attr(&(new->common_attr[(int)common_attr[i].idx]), getXmlAttrByName(attr, common_attr[i].attr_name));

  /* assign object attribute */
  for(i=0; i<OBJ_MAX; i++)
    add_attr(&(new->attr[(int)obj_attr[i].idx]), getXmlAttrByName(attr, obj_attr[i].attr_name));

  //dump_obj(new);	//debug
  insert_obj(&rootObj, new);
  //dump_obj(new);        //debug
  return;
}

/*
 *  endHandler_obj: the handler of object in end phase
 *    input parameter:
 *      userData:
 *      name: element name
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void endHandler_obj(void *userData, const char *name){
  return;
}

/*
 *  startHandler_param: the handler of parameter in start phase
 *    input parameter:
 *      userData:
 *      name: element name
 *      attr: attribut array of element
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void startHandler_param(void *userData, const char *name, const char **attr){
  struct param_entry *new=NULL;
  int i;

  if(!getXmlAttrByName(attr, XML_COMMON_ATTR_NAME)){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_COMMON_ATTR_NAME);
    return;
  }

  if(!isValidStr(param_type, getXmlAttrByName(attr, XML_PARAM_ATTR_TYPE))){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_PARAM_ATTR_TYPE);
    return;
  }

  if(!isValidStr(param_perm, getXmlAttrByName(attr, XML_PARAM_ATTR_PERMISSIN))){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_PARAM_ATTR_PERMISSIN);
    return;
  }

  /* allocate and memset to zero */
  new = (struct param_entry *)calloc(1, sizeof(struct param_entry));
  if(!new){
    printf("[%s]calloc fail\n", __FUNCTION__);
    return;
  }

  /* assign common attribute */
  for(i=0; i<COMMON_MAX; i++)
    add_attr(&(new->common_attr[(int)common_attr[i].idx]), getXmlAttrByName(attr, common_attr[i].attr_name));

  /* assign object attribute */
  for(i=0; i<PARAM_MAX; i++)
    add_attr(&(new->attr[(int)param_attr[i].idx]), getXmlAttrByName(attr, param_attr[i].attr_name));

  //dump_obj(new);      //debug
  insert_param(&(presentObj->param), new);
  //dump_obj(new);        //debug
  return;
}

/*
 *  endHandler_param: the handler of parameter in end phase
 *    input parameter:
 *      userData:
 *      name: element name
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void endHandler_param(void *userData, const char *name){
  return;
}

/*
 *  startHandler_desc: the handler of description in start phase
 *    input parameter:
 *      userData:
 *      name: element name
 *      attr: attribut array of element
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void startHandler_desc(void *userData, const char *name, const char **attr){
  return;
}

/*
 *  endHandler_desc: the handler of description in end phase
 *    input parameter:
 *      userData:
 *      name: element name
 *    output parameter:
 *      NULL
 *    return value:
 *      NULL
 */
void endHandler_desc(void *userData, const char *name){
  return;
}


/* translate to cwmp of Realtek SDK api */
/* type: 0 --> object
 *       1 --> leaf
 */
void genFullOP_FuncName(const int type, const int isGet, const char *name, const int strlen, char *funcName){
  memset(funcName, 0, sizeof(funcName));

  if(type == 0){
    if(isGet){
      snprintf(funcName, strlen, "get"CWMP_OBJ_NAME, name);
      //snprintf(funcName, strlen, "get"CWMP_OBJ_NAME, name);
   }else{
      //set function should only for multiple instance
      snprintf(funcName, strlen, "set"CWMP_OBJ_NAME, name);
      //snprintf(funcName, strlen, "NULL");
   }
  }else{
    if(isGet){
      snprintf(funcName, strlen, "get"CWMP_LEAF_NAME, name);
    }else{
      snprintf(funcName, strlen, "set"CWMP_LEAF_NAME, name);
    }
  }

  return;
}

void translate_Object(struct obj_entry *obj){
  FILE *h_fp=NULL, *c_fp=NULL;
  char h_filename[32], c_filename[32];
  char shortName[MAX_SHORT_NAME];
  char buff[MAX_BUFF_LEN]={0}, getOpBuff[128], setOpBuff[128], objBuff[128], leafBuff[128], tmpObjName[128];
  char typeBuff[32], permBuff[16], tmpbuff[128];
  int offset=0;
  struct obj_entry *tmpObj=NULL;
  struct param_entry *tmpParam=NULL;
  int i,j;

  memset(shortName, 0, sizeof(MAX_SHORT_NAME));
  memset(buff, 0, sizeof(MAX_BUFF_LEN));

  /* translate to one file */
  snprintf(shortName, MAX_SHORT_NAME, "%s", ROOT_OBJ_NAME);

  sprintf(h_filename, H_FILE_NAME, shortName);
  sprintf(c_filename, C_FILE_NAME, shortName);

  if(!((h_fp=fopen(h_filename, "a+")) && (c_fp=fopen(c_filename, "a+")))){
    if(h_fp)
      fclose(h_fp);
    if(c_fp)
      fclose(c_fp);
    return;
  }

  /* step 1: write c file */
  offset = 0;
  memset(buff, 0, sizeof(MAX_BUFF_LEN));

  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** [%s]%s start ********/\n", obj->attr[OBJ_TYPE], obj->attr[OBJ_SHORT_NAME]);

  /**************** leaf info ***********************/
  if(obj->param){
    // root object doesn't need op handler function
    if(strncmp(obj->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME, strlen(obj->attr[OBJ_SHORT_NAME]))){
      /*  [1] write Leaf OP struct
       *   - the handler function when get this function
       */
      genFullOP_FuncName(TYPE_LEAF, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
      genFullOP_FuncName(TYPE_LEAF, FALSE, obj->attr[OBJ_SHORT_NAME], sizeof(setOpBuff), setOpBuff);  //generate set op function name
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_OP_TABLE_ENTRY, obj->attr[OBJ_SHORT_NAME], getOpBuff, setOpBuff);
    }

    /*  [2] write leaf info structure
     *   - list parameter's name/type/flag/op information of this object
     */
    // struct CWMP_PRMT xxxxx[]={
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_INFO_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmpParam = obj->param;
    while(tmpParam){
      for(i=0; i<PARAM_TYPE_NUM; i++){
        if(!strncmp(cwmp_type[i].xml, tmpParam->attr[PARAM_TYPE],strlen(cwmp_type[i].xml))){
          break;
        }
      }

      for(j=0; j<PARAM_PERM_NUM; j++){
        if(!strncmp(cwmp_permission[j].xml, tmpParam->attr[PARAM_PERMISSION],strlen(cwmp_permission[j].xml))){
          break;
        }
      }

      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_INFO_ENTRY,
                              tmpParam->common_attr[COMMON_NAME], cwmp_type[i].cwmp, cwmp_permission[j].cwmp, obj->attr[OBJ_SHORT_NAME]);
      tmpParam = tmpParam->next;    // go through next parameter
      if(tmpParam){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_INFO_END);
  }

  /*  [3] write Leaf Node structure
   *   - list parameter of this object.
   */
  if(obj->param){
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_NODE_BEGIN, obj->attr[OBJ_SHORT_NAME]);

    tmpParam = obj->param;
    while(tmpParam){
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_NODE_ENTRY, obj->attr[OBJ_SHORT_NAME], obj->attr[OBJ_SHORT_NAME], tmpParam->common_attr[COMMON_NAME]);
      tmpParam = tmpParam->next;  // go through next parameter
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");  // { NULL } should be the last entry, which include in CWMP_LEAF_NODE_END
    }

    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_NODE_END);
  }

  /**************** object info ***********************/
  // root object doesn't need op handler function
  if(strncmp(obj->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME, strlen(obj->attr[OBJ_SHORT_NAME]))){
    /*  [1] write Object OP struct
     *   - single object => only get op function
     *   - multiple object => get/set op function
     */
    if(!strncmp(obj->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_SINGLE, strlen(obj->attr[OBJ_TYPE]))){
      genFullOP_FuncName(TYPE_OBJ, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_OP_TABLE_ENTRY, obj->attr[OBJ_SHORT_NAME], getOpBuff, "NULL");
    }else{
      genFullOP_FuncName(TYPE_OBJ, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
      genFullOP_FuncName(TYPE_OBJ, FALSE, obj->attr[OBJ_SHORT_NAME], sizeof(setOpBuff), setOpBuff);  //generate set op function name
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_OP_TABLE_ENTRY, obj->attr[OBJ_SHORT_NAME], getOpBuff, setOpBuff);
    }
  }

  /*  [2] write object info structure
   *   - list sub-object's name/type/flag/op information of this object
   */
  if(obj->child){
    // struct CWMP_PRMT xxxxx[]={
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmpObj = obj->child;
    while(tmpObj){
      if(strncmp(obj->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME, strlen(obj->attr[OBJ_SHORT_NAME]))){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_ENTRY, 
                                tmpObj->attr[OBJ_SHORT_NAME], tmpObj->attr[OBJ_SHORT_NAME]);
      }else{
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_ROOTOBJ_INFO_ENTRY, tmpObj->attr[OBJ_SHORT_NAME]);
      }
      tmpObj = tmpObj->next;	// go through next object
      if(tmpObj){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_END);
  }

  /*  [3] write Object Node structure 
   *   - list sub-object of this object, and which pointer to leaf/object structure of sub-object.
   */
  if(obj->child){
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_NODE_BEGIN, obj->attr[OBJ_SHORT_NAME]);

    tmpObj = obj->child;
    while(tmpObj){
      memset(objBuff, 0, sizeof(objBuff));
      if(!strncmp(tmpObj->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_SINGLE, strlen(tmpObj->attr[OBJ_TYPE])) && tmpObj->child){
	  	/* child object is single object */
        snprintf(objBuff, sizeof(objBuff), CWMP_OBJ_NAME, tmpObj->attr[OBJ_SHORT_NAME]);
      }else{
        /* child is multiple object */
        snprintf(objBuff, sizeof(objBuff), "NULL");  // no child or child object multiple instance
      }

      memset(leafBuff, 0, sizeof(leafBuff));
      if(!strncmp(tmpObj->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_SINGLE, strlen(tmpObj->attr[OBJ_TYPE])) && tmpObj->param){
	  	/* child object is single object */
        snprintf(leafBuff, sizeof(leafBuff), CWMP_LEAF_NAME, tmpObj->attr[OBJ_SHORT_NAME]);
      }else{
        /* child is multiple object */
        snprintf(leafBuff, sizeof(leafBuff), "NULL");	// no leaf or child object multiple instance
      }

      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_NODE_ENTRY, obj->attr[OBJ_SHORT_NAME], tmpObj->attr[OBJ_SHORT_NAME], leafBuff, objBuff);
      tmpObj = tmpObj->next;  // go through next object
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");  // {NULL, NULL, NULL} should be the last entry, which include in CWMP_OBJ_NODE_END
    }

    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_NODE_END);
  }

  /*  for multiple instance object 
   *  multiple instance object should be LNKLIST type
   */
  if(!strncmp(obj->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_MULTIPLE, strlen(obj->attr[OBJ_TYPE]))){
    /* Multiple instance object, it use set op function to init/add/del/modify object instance, add set op function */
    snprintf(tmpObjName, sizeof(tmpObjName), CWMP_OBJ_LINKNODE_NAME, obj->attr[OBJ_SHORT_NAME]);
#if 0  // Multiple instance Object don't need op handler function
    genFullOP_FuncName(TYPE_OBJ, TRUE, tmpObjName, sizeof(getOpBuff), getOpBuff);  //generate get op function name
    genFullOP_FuncName(TYPE_OBJ, FALSE, tmpObjName, sizeof(setOpBuff), setOpBuff);  //generate set op function name
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_OP_TABLE_ENTRY, tmpObjName, getOpBuff, setOpBuff);	   
#endif

    /* current is multiple instance object, write LINKNODE infomation */
    // struct CWMP_PRMT xxxxx[]={
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_BEGIN, tmpObjName);
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_LINK_ENTRY);
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_END);    // };

    /* struct CWMP_LINKNODE xxxxx[]={ */
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_LINKNODE_BEGIN, tmpObjName);
    memset(objBuff, 0, sizeof(objBuff));
    if(obj->child){
      snprintf(objBuff, sizeof(objBuff), CWMP_OBJ_NAME, obj->attr[OBJ_SHORT_NAME]);
    }else{
      /* LNKLIST object without child object */
      snprintf(objBuff, sizeof(objBuff), "NULL");   // no child object
    }

    memset(leafBuff, 0, sizeof(leafBuff));
    if(obj->param){
      snprintf(leafBuff, sizeof(leafBuff), CWMP_LEAF_NAME, obj->attr[OBJ_SHORT_NAME]);
    }else{
      /* LNKLIST object without param */
      snprintf(leafBuff, sizeof(leafBuff), "NULL");   // no leaf
    }

    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_LINKNODE_ENTRY, tmpObjName, leafBuff, objBuff);
//    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_LINKNODE_ENTRY, tmpObjName, obj->attr[OBJ_SHORT_NAME], obj->attr[OBJ_SHORT_NAME]);
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_LINKNODE_END);
  }

  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** %s end ********/\n\n", obj->attr[OBJ_SHORT_NAME]);
//  printf("[ObjInfo Debug]\n%s\n", buff);
  fputs (buff, c_fp);





  /* step 2: write header file */
  offset = 0;
  memset(buff, 0, sizeof(MAX_BUFF_LEN));
  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** [%s]%s start ********/\n", obj->attr[OBJ_TYPE], obj->attr[OBJ_SHORT_NAME]);

  /**************** leaf info ***********************/
  if(obj->param){
    // root object doesn't need op handler function
    if(strncmp(obj->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME, strlen(obj->attr[OBJ_SHORT_NAME]))){
      /*  [1] write Leaf OP prototype */
      genFullOP_FuncName(TYPE_LEAF, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
      genFullOP_FuncName(TYPE_LEAF, FALSE, obj->attr[OBJ_SHORT_NAME], sizeof(setOpBuff), setOpBuff);  //generate get op function name
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_GET_PROTOTYPE, getOpBuff);
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_SET_PROTOTYPE, setOpBuff);
    }

    /*  [2] write leaf info enum */
    // enum e%s_%s{
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_INFO_ENUM_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmpParam = obj->param;
    while(tmpParam){
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "  "CWMP_LEAF_INFO_ENUM_ENTRY,
                              obj->attr[OBJ_SHORT_NAME], tmpParam->common_attr[COMMON_NAME]);
      tmpParam = tmpParam->next;    // go through next parameter
      if(tmpParam){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_LEAF_INFO_ENUM_END);
  }

  /**************** object info ***********************/
  if(!strncmp(obj->attr[OBJ_TYPE], XML_OBJ_ATTR_TYPE_SINGLE, strlen(obj->attr[OBJ_TYPE]))){
    // int %s(char *name, struct CWMP_LEAF *entity, int *type, void **data);
    /* Object is ReadOnly type, so set function assign to "NULL" */
    genFullOP_FuncName(TYPE_OBJ, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_GET_PROTOTYPE, getOpBuff);
  }else{
    genFullOP_FuncName(TYPE_OBJ, TRUE, obj->attr[OBJ_SHORT_NAME], sizeof(getOpBuff), getOpBuff);  //generate get op function name
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_GET_PROTOTYPE, getOpBuff);
    genFullOP_FuncName(TYPE_OBJ, FALSE, obj->attr[OBJ_SHORT_NAME], sizeof(setOpBuff), setOpBuff);  //generate set op function name
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_SET_PROTOTYPE, setOpBuff);
  }

  if(obj->child){
    // enum e%s{
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_ENUM_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmpObj = obj->child;
    while(tmpObj){
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "  "CWMP_OBJ_INFO_ENUM_ENTRY, tmpObj->attr[OBJ_SHORT_NAME]);
      tmpObj = tmpObj->next;    // go through next object
      if(tmpObj){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_ENUM_END);
  }

  if(!strncmp(obj->attr[OBJ_SHORT_NAME], ROOT_OBJ_NAME, strlen(obj->attr[OBJ_SHORT_NAME]))){
    tmpObj = obj->child;
    while(tmpObj){
      snprintf(tmpbuff, sizeof(tmpbuff), "extern struct CWMP_LEAF "CWMP_LEAF_NAME"[];\n", tmpObj->attr[OBJ_SHORT_NAME]);
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "%s", tmpbuff);
      snprintf(tmpbuff, sizeof(tmpbuff), "extern struct CWMP_NODE "CWMP_OBJ_NAME"[];\n", tmpObj->attr[OBJ_SHORT_NAME]);
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "%s", tmpbuff);

      tmpObj = tmpObj->next;    // go through next object      
    }
    snprintf(tmpbuff, sizeof(tmpbuff), "extern struct CWMP_NODE "CWMP_OBJ_NAME"[];\n", obj->attr[OBJ_SHORT_NAME]);
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "%s", tmpbuff);
  }

  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** %s end ********/\n\n", obj->attr[OBJ_SHORT_NAME]);
//  printf("[Obj enum Debug]\n%s\n", buff);
  fputs (buff, h_fp);

  if(h_fp)
    fclose(h_fp);
  if(c_fp)
    fclose(c_fp);

  return;
}

void translate_all_Object(struct obj_entry *initialObj){
  struct obj_entry *currObj=initialObj;
  //printf("[dump_all_Object]rootObj=%p, currObj=%p, initialObj=%p\n", rootObj, currObj, initialObj);

  /* 
   *  1. translate child object first
   *  2. translate next object if no child
   */
  while(currObj != NULL){
    if(currObj->child){
      /* translate child first */
      translate_all_Object(currObj->child);
    }

    translate_Object(currObj);
    currObj = currObj->next;
  }
  return;
}
