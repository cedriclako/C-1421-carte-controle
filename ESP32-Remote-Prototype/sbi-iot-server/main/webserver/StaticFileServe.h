#ifndef _STATICFILESERVE_H_
#define _STATICFILESERVE_H_

#include "Common.h"

#define WSSFS_MIN(a, b) ((a) < (b) ? (a) : (b))

esp_err_t WSSFS_file_get_handler(httpd_req_t *req);

#endif