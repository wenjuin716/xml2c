/* Read an XML document from standard input and print an element
   outline on standard output.
   Must be used with Expat compiled for UTF-8 output.
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 1997-2000 Thai Open Source Software Center Ltd
   Copyright (c) 2000-2017 Expat development team
   Licensed under the MIT license:

   Permission is  hereby granted,  free of charge,  to any  person obtaining
   a  copy  of  this  software   and  associated  documentation  files  (the
   "Software"),  to  deal in  the  Software  without restriction,  including
   without  limitation the  rights  to use,  copy,  modify, merge,  publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons  to whom  the Software  is  furnished to  do so,  subject to  the
   following conditions:

   The above copyright  notice and this permission notice  shall be included
   in all copies or substantial portions of the Software.

   THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
   EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
   NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <expat.h>
#include "xml_translate.h"
#include <unistd.h>  //for getopt
extern char *optarg;
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#ifdef XML_LARGE_SIZE
# if defined(XML_USE_MSC_EXTENSIONS) && _MSC_VER < 1400
#  define XML_FMT_INT_MOD "I64"
# else
#  define XML_FMT_INT_MOD "ll"
# endif
#else
# define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
# define XML_FMT_STR "ls"
#else
# define XML_FMT_STR "s"
#endif

#if 0
struct translate_entry{
  char element[16];		// xml element name
  void (*startHandler)(void *userData, const char *name, const char **atts);	// translate atts to c struct in start phase
  void (*endHandler)(void *userData, const char *name);				// translate atts to c struct in end phase
};
#endif
struct translate_entry xml_translate_table[]={
  { XML_OBJ_ELEMENT, startHandler_obj, endHandler_obj},
  { XML_PARAM_ELEMENT, startHandler_param, endHandler_param},
  { XML_DESC_ELEMENT, startHandler_desc, endHandler_desc}
}; 

#define xmlTranslateNum		((int)(sizeof(xml_translate_table)/sizeof(struct translate_entry)))
extern struct obj_entry *rootObj;

int Depth;

static void XMLCALL
start(void *data, const XML_Char *el, const XML_Char **attr)
{
#if 1
  int i;

  for(i=0; i<xmlTranslateNum; i++){
    if(!strncmp(el, xml_translate_table[i].element, strlen(el))){
	xml_translate_table[i].startHandler(data, (const char*)el, (const char**)attr);
    }
  }
  Depth++;
#else
  int i;
  (void)data;
  for (i = 0; i < Depth; i++)
    printf("  ");

  printf("%" XML_FMT_STR, el);

  for (i = 0; attr[i]; i += 2) {
    printf(" %" XML_FMT_STR "='%" XML_FMT_STR "'", attr[i], attr[i + 1]);
  }

  printf("\n");
  Depth++;
#endif
}

static void XMLCALL
end(void *data, const XML_Char *el)
{
  (void)data;
  (void)el;
  int i;

  for(i=0; i<xmlTranslateNum; i++){
    if(!strncmp(el, xml_translate_table[i].element, strlen(el))){
        xml_translate_table[i].endHandler(data, (const char*)el);
    }
  }

  Depth--;
}

static void XMLCALL
CharacterDataHandler(void *userData, const XML_Char *s, int len){
 printf("[%s] len=%d\n", __FUNCTION__, len);
}

static void XMLCALL
ProcessingInstructionHandler(void *userData, const XML_Char *target, const XML_Char *data){
  printf("[%s] target=%s\n", __FUNCTION__, target);
  printf("[%s] data=%s\n", __FUNCTION__, data);
}

void usage(void){
  fprintf(stderr, "xml2c - translate cwmp xml file to c format\n");
  fprintf(stderr, "  -f <FILE>      cwmp xml file\n");
  fprintf(stderr, "  -d             debug mode(OPTION)\n");
  return;
}

int
main(int argc, char *argv[])
{
  int c;	/* command line arg */
  char xmlName[64]={0}, *xml_mem=NULL;
  int xml_fd;
  struct stat xml_stat;

  while ((c = getopt(argc, argv, "f:dh")) != -1){
    switch (c) {
      case 'f':
        snprintf(xmlName, sizeof(xmlName), "%s", optarg);
        break;
      case 'd':
        debug=TRUE;
        break;
      case 'h':
        usage();
        exit(0);
      default:
        fprintf(stderr, "known arg option (%c)\n", (char)c);
	usage();
        exit(-1);
    }
  }

  if(-1 == (xml_fd = open(xmlName, O_RDONLY))){
    fprintf(stderr, "open %s error\n", xmlName);
    exit(-1);
  }

  fstat(xml_fd, &xml_stat);
  /* MAP_OPTIONS: see compat.h */
  if((void *)-1 == (xml_mem = mmap(0, xml_stat.st_size, PROT_READ, MAP_FILE|MAP_PRIVATE, xml_fd, 0))){
    fprintf(stderr, "mmap error\n");
    exit(-1);
  }
  close(xml_fd);

  //DEBUG("xml_mem=\n%s\n", xml_mem);

  XML_Parser p = XML_ParserCreate(NULL);

  if (! p) {
    fprintf(stderr, "Couldn't allocate memory for parser\n");
    exit(-1);
  }

  initXml2c();

  XML_SetElementHandler(p, start, end);
//  XML_SetCharacterDataHandler(p, CharacterDataHandler);
//  XML_SetProcessingInstructionHandler(p, ProcessingInstructionHandler);

  if (XML_Parse(p, xml_mem, xml_stat.st_size, 1) == XML_STATUS_ERROR) {
    fprintf(stderr,
            "Parse error at line %" XML_FMT_INT_MOD "u:\n%" XML_FMT_STR "\n",
            XML_GetCurrentLineNumber(p),
            XML_ErrorString(XML_GetErrorCode(p)));
    exit(-1);
  }

  // XML file parser done, insert root obj to the head.
  insert_rootObj();  //depth order error
  //dump_all_Object(rootObj);

  initCWMP_File();
  translate_all_Object(rootObj);
  XML_ParserFree(p);
  return 0;
}
