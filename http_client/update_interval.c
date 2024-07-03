//
// Created by 宋义宝 on 2024/7/3.
//
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>
#include "../collector.h"
#include "update_interval.h"

// URL编码函数
char *url_encode(const char *str) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }
    char *encoded_str = curl_easy_escape(curl, str, 0);
    curl_easy_cleanup(curl);
    return encoded_str;
}

// 回调函数，用于将响应数据写入内存
static size_t get_subscribed_group_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    neu_plugin_t *plugin = (neu_plugin_t *) userp;
    // {"groups": [{"driver": "西门子plc", "group": "test"}]}
    // use yyjson to parse json
    yyjson_doc *doc = yyjson_read((char *) contents, strlen((char *) contents), 0);
    if (doc == NULL) {
        printf("doc is null\n");
        goto exit;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *groups = yyjson_obj_get(root, "groups");
    if (groups == NULL) {
        printf("groups is null\n");
        goto exit;
    }
    yyjson_val *group = yyjson_arr_get(groups, 0);
    if (group == NULL) {
        printf("group is null\n");
        goto exit;
    }
    yyjson_val *driver = yyjson_obj_get(group, "driver");
    if (driver == NULL) {
        printf("driver is null\n");
        goto exit;
    }
    yyjson_val *group_name = yyjson_obj_get(group, "group");
    if (group_name == NULL) {
        printf("group_name is null\n");
        goto exit;
    }
    strcpy(plugin->node_name, yyjson_get_str(driver));
    strcpy(plugin->group_name, yyjson_get_str(group_name));
    plugin->subscribed = true;
    get_node_group_interval(plugin->node_name, plugin->group_name, plugin);
    exit:
    return size * nmemb;
}

static size_t get_node_group_interval_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    neu_plugin_t *plugin = (neu_plugin_t *) userp;
    // {"groups": [{"tag_count": 5, "name": "test", "interval": 1000}]}
    // use yyjson to parse json
    yyjson_doc *doc = yyjson_read((char *) contents, strlen((char *) contents), 0);
    if (doc == NULL) {
        printf("doc is null\n");
        goto exit;
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *groups = yyjson_obj_get(root, "groups");
    if (groups == NULL) {
        printf("groups is null\n");
        goto exit;
    }
    yyjson_val *group = yyjson_arr_get(groups, 0);
    if (group == NULL) {
        printf("group is null\n");
        goto exit;
    }
    yyjson_val *interval = yyjson_obj_get(group, "interval");
    if (interval == NULL) {
        printf("interval is null\n");
        goto exit;
    }
    plugin->node_group_interval = yyjson_get_int(interval);
    exit:
    return size * nmemb;
}

void get_subscribed_group(const char *node, neu_plugin_t *plugin) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        char url[256];
        const char *base_url = "http://127.0.0.1:7000/api/v2/subscribe?app=%s";
        char *encoded_node = url_encode(node);
        snprintf(url, strlen(base_url) + strlen(encoded_node), base_url, encoded_node);
        free(encoded_node);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_subscribed_group_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) plugin);
        // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        // curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,
                                    "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw");
        headers = curl_slist_append(headers, "User-Agent: Apifox/1.0.0 (https://apifox.com)");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
}

void get_node_group_interval(const char *node, const char *group, neu_plugin_t *plugin) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        char url[256];
        const char *base_url = "http://127.0.0.1:7000/api/v2/group?node=%s";
        char *encoded_node = url_encode(node);
        snprintf(url, strlen(base_url) + strlen(encoded_node), base_url, encoded_node);
        free(encoded_node);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_node_group_interval_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) plugin);
        // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        // curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,
                                    "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw");
        headers = curl_slist_append(headers, "User-Agent: Apifox/1.0.0 (https://apifox.com)");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        res = curl_easy_perform(curl);
    }
    curl_easy_cleanup(curl);
}

char *params_to_string(const char *node, const char *group, int interval) {
    char *str = (char *) malloc(100);
    sprintf(str, "{\"node\":\"%s\",\"group\":\"%s\",\"interval\":%d}", node,
            group, interval);
    return str;
}

int update_interval(char *node, char *group, int interval, neu_plugin_t *plugin) {
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_URL,
                         "http://127.0.0.1:7000/api/v2/group");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers,
                                    "Authorization: Bearer eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJieXdnIiwiaWF0IjoiMTcxOTkyNDUxMSIsImV4cCI6IjE3ODI5OTY0NTQiLCJhdWQiOiJuZXVyb24iLCJib2R5RW5jb2RlIjoiMCJ9.eQGzzOF10cgav7dO1rdpMnSyQtZtCmzKDOuLPbLYAQzfOteifLWGM6dD3QoBb1-oD6HLqabouMVn9LMwbeV5mnyOgKFbCNzIwke6N6pqrtd_500bZQDmSIYCDytZkXWj4__g4Zy5oPCK0Xfz8n-w4bLNKzGK-Uo7nxMIfBvxyNhyqth7g8UZebcUJwxECaHluUuocWkS6iD-_rWcIR3cbC7oWWryFvC0ZE34BmkHDXNBtL6yL_eg5XXHOzjQynLOvVG_EXBKKrhdVZeBrFiykpeSE4Uo5REZZUtJ0BPwN6n8kjPQzCA3k9x0JIMSvN7FOob0X3-BGSzpqjBwFzSEPw");
        headers = curl_slist_append(headers,
                                    "User-Agent: Apifox/1.0.0 (https://apifox.com)");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        char *data = params_to_string(node, group, interval);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            plugin->node_group_interval = interval;
        }
        free(data);
    }
    curl_easy_cleanup(curl);

    return 0;
}
// update_interval(plugin->node_name, plugin->group_name, plugin->node_group_interval + 1, plugin);
