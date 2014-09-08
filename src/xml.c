
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <gtk/gtk.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include "mylocale.h"
#include "xml.h"
#include "main.h"
#include "utils.h"

extern Config cfg;

static char *flag_value[] = {"notification", "status-icon", "auto"};

int write_config(void) {
  int rc, result = 0;
  char *config_name = ".internet_icon";
  xmlTextWriterPtr writer;
#ifdef MEMORY
  int fd;
  xmlBufferPtr buf;

  /* Create a new XML buffer, to which the XML document will be
   * written */
  buf = xmlBufferCreate();
  if (buf == NULL) {
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
  writer = xmlNewTextWriterFilename(config_name, 0);
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
  // timeout
  char *tm = mysprintf ("%d", cfg.timeout_seconds);
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "timeout", BAD_CAST tm);
  free (tm);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  // test_ip
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "test_ip", BAD_CAST cfg.test_ip);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  // the port
  char *port = mysprintf ("%d", cfg.test_port);
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "test_port", BAD_CAST port);
  free (port);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  // operating mode
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "opmode", BAD_CAST flag_value[cfg.op_mode]);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  // wan ip page
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "wan_ip_page", BAD_CAST cfg.wanip_page);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
  if (rc < 0) goto finish;
  // wan ip page
  rc = xmlTextWriterWriteElement(writer, BAD_CAST "user_agent", BAD_CAST cfg.user_agent);
  if (rc < 0) goto finish;
  // newline
  rc = xmlTextWriterWriteString(writer, BAD_CAST "\n");
  if (rc < 0) goto finish;
  /* Close the start element. */
  rc = xmlTextWriterEndElement(writer);
  if (rc < 0) goto finish;
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
  write(fd, (const void *) buf->content, strlen((const char *)buf->content));
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

int get_flag(const char *s) {
  int i;
  for (i = 0; i < _ENDFLAGS; i++)
    if (strcasecmp (s, flag_value[i]) == 0)
      return i;
  return _AUTO;
}

void
parse_config(void) {

  xmlDocPtr doc;
  xmlNodePtr cur, nodeptr;
  char *home = getenv("HOME");

  if (!(home && !chdir(home))) {
    fprintf(stderr, _("Can't chdir to home\n"));
    return;
  }

  doc = xmlParseFile(".internet_icon");
  if (doc == NULL) {
    if (!(write_config())) {
      fprintf(stderr, _("Can't create configuration file\n"));
    }
    return;
  }

  cur = nodeptr = xmlDocGetRootElement(doc);

  if (cur == NULL) {
    xmlFreeDoc(doc);
    fprintf(stderr, _("Configuration: empty document\n"));
    return;
  }

  if (xmlStrcmp(cur->name, (const xmlChar *) "internet_icon") != 0) {
    xmlFreeDoc(doc);
    fprintf(stderr, _("Configuration: document of the wrong type, root node != internet_icon\n"));
    return;
  }

  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "timeout"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.timeout_seconds = atoi((const char *)key);
        if (cfg.timeout_seconds < 10)
          cfg.timeout_seconds = 10;
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "test_ip"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.test_ip = (char *)key;
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "test_port"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.test_port = atoi((const char *)key);
        if (cfg.test_port < 1)
          cfg.test_port = 80;
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "opmode"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.op_mode = get_flag((const char *)key);
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "wan_ip_page"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.wanip_page = (char *)key;
      }
    }
    if ((!xmlStrcmp(cur->name, (const xmlChar *) "user_agent"))) {
      xmlChar *key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key && *key) {
        cfg.user_agent = (char *)key;
      }
    }
    cur = cur->next;
  }

  xmlFreeDoc(doc);
  return;
}

