#include "StaticFileServe.h"

#define TAG "StaticFileServe"

static const EF_SFile* GetFileByURL(const char* strFilename, EF_EFILE* pOutEFile);
static const EF_SFile* GetFile(EF_EFILE eFile);
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename);

/* An HTTP GET handler */
esp_err_t WSSFS_file_get_handler(httpd_req_t *req)
{
    const EF_SFile* pFile = NULL;
    EF_EFILE eFile;

    ESP_LOGI(TAG, "Opening file uri: '%s'", req->uri);
    
    if (strcmp(req->uri, "/") == 0 || strcmp(req->uri, DEFAULT_RELATIVE_URI) == 0) {
        // Redirect root to index.html
        eFile = EF_EFILE_USER_INDEX_HTML;
        pFile = GetFile(eFile);
    }
    else {
        pFile = GetFileByURL(req->uri+1, &eFile);
    }

    if (pFile == NULL)
    {
        httpd_resp_set_hdr(req, "Connection", "close");
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File does not exist");
        return ESP_FAIL;
    }
    else if (eFile == EF_EFILE_MNT_INDEX_HTML)
    {
        CHECK_FOR_ACCESS_OR_RETURN();
    }

    set_content_type_from_file(req, pFile->strFilename);

    uint32_t u32Index = 0;

    while(u32Index < pFile->u32Length)
    {
        const uint32_t n = WSSFS_MIN(pFile->u32Length - u32Index, HTTPSERVER_BUFFERSIZE);

        if (n > 0) {
            /* Send the buffer contents as HTTP response g_u8Buffers */
            if (httpd_resp_send_chunk(req, (char*)(pFile->pu8StartAddr + u32Index), n) != ESP_OK) {
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
           }
        }
        u32Index += n;
    }

    httpd_resp_set_hdr(req, "Connection", "close");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)
    
/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filename)
{
    if (IS_FILE_EXT(filename, ".pdf")) {
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html") | IS_FILE_EXT(filename, ".htm")) {
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg") || IS_FILE_EXT(filename, ".jpg")) {
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")) {
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".css")) {
        return httpd_resp_set_type(req, "text/css");
    } else if (IS_FILE_EXT(filename, ".txt")) {
        return httpd_resp_set_type(req, "text/plain");
    } else if (IS_FILE_EXT(filename, ".js")) {
        return httpd_resp_set_type(req, "text/javascript");
    } else if (IS_FILE_EXT(filename, ".json")) {
        return httpd_resp_set_type(req, "application/json");
    }
    else if (IS_FILE_EXT(filename, ".ttf")) {
        return httpd_resp_set_type(req, "application/x-font-truetype");
    }
    else if (IS_FILE_EXT(filename, ".woff")) {
        return httpd_resp_set_type(req, "application/font-woff");
    }
    else if (IS_FILE_EXT(filename, ".svg")) {
        return httpd_resp_set_type(req, "image/svg+xml");
    }
    
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

static const EF_SFile* GetFileByURL(const char* strFilename, EF_EFILE* pOutEFile)
{
    for(int i = 0; i < EF_EFILE_COUNT; i++)
    {
        const EF_SFile* pFile = &EF_g_sFiles[i];
        if (strcmp(pFile->strFilename, strFilename) == 0)
        {
            if (pOutEFile) {
                *pOutEFile = (EF_EFILE)i;
            }
            return pFile; 
        }
    }

    return NULL;
}

static const EF_SFile* GetFile(EF_EFILE eFile)
{
    if (eFile >= EF_EFILE_COUNT)
        return NULL;
    const EF_SFile* pFile = &EF_g_sFiles[(int)eFile];
    return pFile;
}
