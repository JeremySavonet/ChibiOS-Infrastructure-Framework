
#include "ch.h"
#include "hal.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/err.h"

#include "Modules/SimpleWebserver/simpleWebserver.hpp"


static const char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
static const char http_index_html[] = "<html><head><title>Congrats!</title></head><body><h1>Welcome to our lwIP HTTP server!</h1><p>This is a small test page.</body></html>";

static void http_server_serve(struct netconn *conn) {
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);

  if (err == ERR_OK) {
    netbuf_data(inbuf, (void **)&buf, &buflen);

    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    if (buflen>=5 &&
        buf[0]=='G' &&
        buf[1]=='E' &&
        buf[2]=='T' &&
        buf[3]==' ' &&
        buf[4]=='/' ) {

      /* Send the HTML header
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
       */
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

      /* Send our HTML page */
      netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


namespace SimpleWebserver {

msg_t SimpleWebserver::init(){
  // No need to do anything for initialization.
  return RDY_OK;
}

msg_t SimpleWebserver::Main(){

  chRegSetThreadName("simpleWebserver");

  struct netconn *conn, *newconn;
  err_t err;

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  LWIP_ERROR("http_server: invalid conn", (conn != NULL), return RDY_RESET;);

  /* Bind to port 80 (HTTP) with default IP address */
  netconn_bind(conn, NULL, WEB_THREAD_PORT);

  /* Put the connection into LISTEN state */
  netconn_listen(conn);

  /* Goes to the final priority after initialization.*/
  chThdSetPriority(WEB_THREAD_PRIORITY);

  while(1) {
    err = netconn_accept(conn, &newconn);
    if (err != ERR_OK)
      continue;
    http_server_serve(newconn);
    netconn_delete(newconn);
  }


  return RDY_OK;
}

}

