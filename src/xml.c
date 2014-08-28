
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <locale.h>
#include <libintl.h>
#include "mylocale.h"
#include "xml.h"
#include "main.h"
#include "utils.h"

extern Config cfg;

int write_config(void) {
  int rc, result = 0;
  char *config_name = ".internet_icon.conf";
  xmlTextWriterPtr writer;
#ifdef MEMORY
  int fd;
  xmlBufferPtr buf;

  /* Create a new XML buffer, to which the XML document will be
   * written */
  buf = xmlBufferCreate();
  if (buf == NULL) {
    //printf("testXmlwriterMemory: perror creating the xml buffer\n");
    return 0;
  }

  /* Create a new XmlWriter for memory, with no compression.
   * Remark: there is no compression for this kind of xmlTextWriter */
  writer = xmlNewTextWriterMemory(buf, 0);
  if (writer == NULL) {
    xmlBufferFree(buf);
    return 0;
  }
#else
  // create the file
  writer = xmlNewTextWriterFilename(globaldata.gd_config_root ? CONFIG_NAME_ROOT : CONFIG_NAME, 0);
  if (writer == NULL) return 0;
#endif

  // start the document
  rc = xmlTextWriterStartDocument(writer, NULL, NULL, NULL);
  if (rc < 0) goto finish;
  // root element
  rc = xmlTextWriterStartElement(writer, BAD_CAST "internet_icon");
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  /////////////////////////////////////
  // the timeout
  char *tm = mysprintf ("%d", cfg.timeout_seconds);
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "timeout", BAD_CAST tm);
  free (tm);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  /* Close the start element. */
  rc = xmlTextWriterEndElement(writer);
  /////////////////////////////////////
  /* Close all. */
  rc = xmlTextWriterEndDocument(writer);
  if (rc < 0) goto finish;

#ifdef MEMORY
  xmlFreeTextWriter(writer);
  writer = NULL;

  remove(config_name);
  fd = open(config_name, O_CREAT | O_RDWR | O_TRUNC, 0600);
  if (fd < 0) goto finish;
  write(fd, (const void *) buf->content, strlen(buf->content));
  close(fd);
#endif

  result = 1;

finish:
  if (writer) xmlFreeTextWriter(writer);
#ifdef MEMORY
  xmlBufferFree(buf);
#endif
  return result;
}

void
parse_config(void) {

  xmlDocPtr doc;
  xmlNodePtr cur, nodeptr;
  char *home = getenv("HOME");

  if (!(home && !chdir(home))) {
    perror(_("Can't chdir to home"));
    return;
  }

  doc = xmlParseFile(".internet_icon.conf");
  if (doc == NULL) {
    perror(_("Configuration xml document not parsed successfully. \n"));
    // test if success
    write_config();
    return;
  }

  cur = nodeptr = xmlDocGetRootElement(doc);

  if (cur == NULL) {
    xmlFreeDoc(doc);
    perror(_("Configuration: empty document\n"));
    return;
  }

  if (xmlStrcmp(cur->name, (const xmlChar *) "internet_icon") != 0) {
    xmlFreeDoc(doc);
    perror(_("Configuration: document of the wrong type, root node != internet_icon"));
    return;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "timeout"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key)
        cfg.timeout_seconds = atoi(key);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "user"))) {
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "mysql"))) {
    }
    cur = cur->next;
  }

  xmlFreeDoc(doc);
  return;
}

#ifdef XYZ
int parseMainNode(xmlDocPtr doc, xmlNodePtr cur, int action) {
  xmlChar *key, *attrib, *message;
  cur = cur->xmlChildrenNode;
  INIDATA *inidata;

  globaldata.gd_inidata = inidata = calloc(sizeof (INIDATA), 1);
  if (inidata == NULL) return 0;
  inidata->flags |= (_SKIP_MYSQL | _SKIP_CONFIGFILE);

  while (cur != NULL) {
    if ((!xmlStrcmp(cur->name, (xmlChar *) "directory"))) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      attrib = xmlGetProp(cur, (xmlChar *) "create");
      message = xmlGetProp(cur, (xmlChar *) "message");
      inidata->directory = strdup((char *) key);
      inidata->dir_msg = message ? strdup((char *)message) : NULL;
      if (xmlStrcmp(attrib, (xmlChar *) "yes") == 0) {
        inidata->flags |= _CREATEDIR;
      }
      xmlFree(key);
      if (attrib) xmlFree(attrib);
    }
    if ((!xmlStrcmp(cur->name, (xmlChar *) "url"))) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      inidata->web_archive = (char *)key;
    }
    if ((!xmlStrcmp(cur->name, (xmlChar *) "file"))) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      attrib = xmlGetProp(cur, (xmlChar *) "unzip");
      if (globaldata.gd_iniaddress && !inidata->web_archive)
        inidata->web_archive = cloneaddress(globaldata.gd_iniaddress, (char *) key);
      else if (action == UPLOAD_CONFIG) { // upload, not download
        char *path = getenv("DOCUMENT_ROOT");
        if (path && *path) {
          path = append_cstring(NULL, path);
          if (path[strlen(path) - 1] != '/') path = append_cstring(path, "/");
          inidata->web_archive = append_cstring(path, (char *) key);
        }
      }
      inidata->zip_format = setUnzip((char *) key, attrib ? (char *) attrib : "auto");
      xmlFree(key);
      if (attrib) xmlFree(attrib);
    }
    cur = cur->next;
  }
  return 1;

}

int read_xml_file(int action) {
  xmlDocPtr doc;
  xmlNodePtr cur, nodeptr;

  doc = xmlParseFile(globaldata.gd_inifile);

  if (doc == NULL) {
    perror(_("Configuration xml document not parsed successfully. \n"));
  }

  cur = nodeptr = xmlDocGetRootElement(doc);

  if (cur == NULL) {
    xmlFreeDoc(doc);
    perror(_("Configuration: empty document\n"));
  }

  if (xmlStrcmp(cur->name, (const xmlChar *) "ezinstaller") != 0) {
    xmlFreeDoc(doc);
    perror(_("Configuration: document of the wrong type, root node != ezinstaller"));
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "main"))) {
      parseMainNode(doc, cur, action);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "permissions"))) {
      parsePermissionsNode(doc, cur);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "config"))) {
      parseConfigNode(doc, cur);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "mysql"))) {
      parseMysqlNode(doc, cur);
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "finish"))) {
      parseFinishNode(doc, cur);
    }
    cur = cur->next;
  }

  return 1;
}


#endif
