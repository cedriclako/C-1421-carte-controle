#ifndef _APIPOST_H_
#define _APIPOST_H_

#include "Common.h"

esp_err_t APIPOST_post_handler(httpd_req_t *req);

esp_err_t APIPOST_action_post_handler(httpd_req_t *req);

#endif