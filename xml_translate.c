#include "xml_translate.h"

#define MAX_BUFF_LEN 10000	// Max buff length should be tIGDObject[] of TR98
#define MAX_SHORT_NAME 64	// Max object short name length

struct obj_entry *rootObj=NULL;	// the root object for this spec
struct obj_entry *presentObj=NULL; // current Object

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
    }else if(currObj->next){
      dump_all_Object(currObj->next);
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
    }else if(currObj->next){
      resort_depth(currObj->next, isAdd);
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
/******************* linked list api ******************/

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
  int i;

  for (i = 0; attr[i]; i += 2) {
    if(!strncmp(name, attr[i], strlen((const char*)name))){
      /* found target */
      return attr[i+1];
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

  if(!getXmlAttrByName(attr, XML_OBJ_ATTR_OBJ_TYPE)){
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

  if(!getXmlAttrByName(attr, XML_PARAM_ATTR_TYPE)){
    printf("[%s]element %s is NULL\n", __FUNCTION__, XML_PARAM_ATTR_TYPE);
    return;
  }

  if(!getXmlAttrByName(attr, XML_PARAM_ATTR_PERMISSIN)){
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
void genFullOP_FuncName(const int type, const char *name, const int strlen, char *funcName){
  memset(funcName, 0, sizeof(funcName));

  if(type == 0){
    /* for get op function */
    snprintf(funcName, strlen, "get%s", name);
  }else{
    snprintf(funcName, strlen, "set%s", name);
  }

  return;
}

void translate_ObjInfo(struct obj_entry *obj){
  FILE *h_fp=NULL, *c_fp=NULL;
  char h_filename[32], c_filename[32];
  char shortName[MAX_SHORT_NAME];
  char buff[MAX_BUFF_LEN]={0}, tmpBuff[128];
  int offset=0;
  struct obj_entry *tmp=NULL;

  memset(shortName, 0, sizeof(MAX_SHORT_NAME));
  memset(buff, 0, sizeof(MAX_BUFF_LEN));

  /* translate to one file */
  snprintf(shortName, MAX_SHORT_NAME, "%s", ROOT_OBJ_NAME);

  sprintf(h_filename, h_file_name, shortName);
  sprintf(c_filename, c_file_name, shortName);

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

  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** %s start ********/\n", obj->attr[OBJ_SHORT_NAME]);

  // struct CWMP_OP &t%sOP = { %s, %s };
  /* Object is ReadOnly type, so set function assign to "NULL" */
  genFullOP_FuncName(0, obj->attr[OBJ_SHORT_NAME], sizeof(tmpBuff), tmpBuff);  //generate get op function name
  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OP_TABLE_ENTRY, obj->attr[OBJ_SHORT_NAME], tmpBuff, "NULL");

  if(obj->child){
    // struct CWMP_PRMT %sInfo[]={
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmp = obj->child;
    while(tmp){
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_ENTRY, 
                              tmp->attr[OBJ_SHORT_NAME], tmp->attr[OBJ_SHORT_NAME]);
      tmp = tmp->next;	// go through next object
      if(tmp){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_INFO_END);
  }

  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** %s end ********/\n\n", obj->attr[OBJ_SHORT_NAME]);
//  printf("[ObjInfo Debug]\n%s\n", buff);
  fputs (buff, c_fp);





  /* step 2: write header file */
  offset = 0;
  memset(buff, 0, sizeof(MAX_BUFF_LEN));
  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), "/******** %s start ********/\n", obj->attr[OBJ_SHORT_NAME]);

  // int %s(char *name, struct CWMP_LEAF *entity, int *type, void **data);
  /* Object is ReadOnly type, so set function assign to "NULL" */
  genFullOP_FuncName(0, obj->attr[OBJ_SHORT_NAME], sizeof(tmpBuff), tmpBuff);  //generate get op function name
  offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_GET_PROTOTYPE, tmpBuff);

  if(obj->child){
    // enum e%s{
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_ENUM_BEGIN, obj->attr[OBJ_SHORT_NAME]);
    tmp = obj->child;
    while(tmp){
      offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_OBJ_ENUM_ENTRY, tmp->attr[OBJ_SHORT_NAME]);
      tmp = tmp->next;    // go through next object
      if(tmp){
        offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), ",\n");
      }
    }
    // };
    offset += snprintf(buff+offset, (MAX_BUFF_LEN-offset), CWMP_ENUM_END);
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

    translate_ObjInfo(currObj);
    currObj = currObj->next;
  }
  return;
}
