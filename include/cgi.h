#ifndef CGI_H
#define CGI_H

#include "lwip/apps/httpd.h"

class Write_Flash;

extern Write_Flash write_flash;

void cgi_init(void);

#endif // CGI_H