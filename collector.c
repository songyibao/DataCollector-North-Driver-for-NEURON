//
// Created by root on 5/23/24.
//
#include "collector.h"
#include "http_client/update_interval.h"
#include "service/service.h"

#define DESCRIPTION "Plugin responsible for data collection and storage"
#define DESCRIPTION_ZH "负责数据采集和存储的插件"
static const neu_plugin_intf_funs_t plugin_intf_funs = {
    .open = driver_open,
    .close = driver_close,
    .init = driver_init,
    .uninit = driver_uninit,
    .start = driver_start,
    .stop = driver_stop,
    .setting = driver_config,
    .request = driver_request,

    .driver.validate_tag = driver_validate_tag,
    .driver.group_timer = driver_group_timer,
    .driver.write_tag = driver_write,
};

const neu_plugin_module_t neu_plugin_module = {
    .version = NEURON_PLUGIN_VER_1_0,
    .schema = "collector",
    .module_name = "Data Collector",
    .module_descr = DESCRIPTION,
    .module_descr_zh = DESCRIPTION_ZH,
    .intf_funs = &plugin_intf_funs,
    .kind = NEU_PLUGIN_KIND_SYSTEM,
    .type = NEU_NA_TYPE_APP,
    .display = true,
    .single = false,
};

static neu_plugin_t *driver_open(void) {
    neu_plugin_t *plugin = calloc(1, sizeof(neu_plugin_t));

    neu_plugin_common_init(&plugin->common);
    plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;

    return plugin;
}

static int driver_close(neu_plugin_t *plugin) {
    free(plugin);

    return 0;
}

// driver_init -> driver_config -> driver_start
static int driver_init(neu_plugin_t *plugin, bool load) {
    (void)load;
    plog_notice(plugin,
                "============================================================"
                "\ninitialize "
                "plugin============================================================\n");
    plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
    plugin->started = false;
    plugin->fp = NULL;
    plugin->pre_str = NULL;
    return 0;
}

static int driver_config(neu_plugin_t *plugin, const char *setting) {
    plog_notice(plugin,
                "============================================================\nconfig "
                "plugin============================================================\n");
    int ret = 0;
    char *err_param = NULL;

    neu_json_elem_t path = {.name = "path", .t = NEU_JSON_STR, .v.val_str = NULL};
    neu_json_elem_t precision = {.name = "precision", .t = NEU_JSON_INT};
    neu_json_elem_t with_timestamp = {.name = "with_timestamp", .t = NEU_JSON_BOOL};
    neu_json_elem_t deduplication = {.name = "deduplication", .t = NEU_JSON_BOOL};
    neu_json_elem_t need_dynamic_interval = {.name = "dynamic_interval", .t = NEU_JSON_BOOL};
    neu_json_elem_t max_file_size = {.name = "max_file_size", .t = NEU_JSON_INT};
    ret = neu_parse_param((char *)setting, &err_param, 6, &path, &precision, &with_timestamp,
                          &deduplication, &max_file_size, &need_dynamic_interval);
    plugin->precision = 1;
    for (int i = 0; i < (int)precision.v.val_int; i++) {
        plugin->precision = plugin->precision * 10;
    }
    plugin->with_timestamp = with_timestamp.v.val_bool;
    plugin->need_deduplication = deduplication.v.val_bool;
    plugin->need_dynamic_interval = need_dynamic_interval.v.val_bool;
    if (plugin->need_deduplication == false && plugin->pre_str != NULL) {
        free(plugin->pre_str);
        plugin->pre_str = NULL;
    }
    plugin->max_file_size = max_file_size.v.val_int * 1024 * 1024;
    strcpy(plugin->path, path.v.val_str);

    plog_notice(plugin, "config: path: %s", plugin->path);
    return 0;
}

static int driver_start(neu_plugin_t *plugin) {
    plog_notice(plugin,
                "============================================================\nstart "
                "plugin============================================================\n");
    plugin->started = true;
    if (plugin->pre_str != NULL) {
        free(plugin->pre_str);
        plugin->pre_str = NULL;
    }
    // plugin->path/data_时间戳.csv 作为文件名
    char filename[290];
    long timestamp = neu_time_ms();
    sprintf(filename, "%s/data_%ld.csv", plugin->path, timestamp);
    plugin->fp = fopen(filename, "w");
    if (plugin->fp == NULL) {
        return NEU_ERR_FILE_OPEN_FAILURE;
    }
    return 0;
}

static int driver_stop(neu_plugin_t *plugin) {
    plog_notice(plugin,
                "============================================================\nstop "
                "plugin============================================================\n");
    plugin->started = false;
    if (plugin->fp != NULL) {
        fclose(plugin->fp);
        plugin->fp = NULL;
    }
    if (plugin->pre_str != NULL) {
        free(plugin->pre_str);
        plugin->pre_str = NULL;
    }
    return 0;
}

static int driver_uninit(neu_plugin_t *plugin) {
    plog_notice(plugin,
                "============================================================\nuninit "
                "plugin============================================================\n");

    if (plugin->fp != NULL) {
        fclose(plugin->fp);
        plugin->fp = NULL;
    }
    return NEU_ERR_SUCCESS;
}

static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head, void *data) {
    plog_notice(plugin,
                "============================================================request "
                "plugin============================================================\n");
    plog_debug(plugin, "request type: %d", head->type);

    switch (head->type) {
        case NEU_REQRESP_TRANS_DATA:
            if (plugin->started == false) {
                return 0;
            }
            handle_trans_data(plugin, head, data);
            break;
        case NEU_REQ_SUBSCRIBE_GROUP:
            //            typedef struct {
            //                char     app[NEU_NODE_NAME_LEN];
            //                char     driver[NEU_NODE_NAME_LEN];
            //                char     group[NEU_GROUP_NAME_LEN];
            //                uint16_t port;
            //                char *   params;
            //            } neu_req_subscribe_t;
            //
            neu_req_subscribe_t *req_data = (neu_req_subscribe_t *)data;
            // log the request
            plog_notice(plugin, "app [%s] subscribe [%s]'s group: [%s]", req_data->app,
                        req_data->driver, req_data->group);
            strcpy(plugin->node_name, req_data->driver);
            strcpy(plugin->group_name, req_data->group);
            plugin->subscribed = true;
            get_node_group_interval(plugin->node_name, plugin->group_name, plugin);
            plog_debug(plugin, "此订阅组的采集间隔: %d", plugin->node_group_interval);
        case NEU_REQ_UNSUBSCRIBE_GROUP:
            //            typedef struct {
            //                char app[NEU_NODE_NAME_LEN];
            //                char driver[NEU_NODE_NAME_LEN];
            //                char group[NEU_GROUP_NAME_LEN];
            //            } neu_req_unsubscribe_t;
            plugin->subscribed = false;
        default:
            break;
    }
    return 0;
}

static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag) {
    plog_notice(plugin, "validate tag: %s", tag->name);

    return 0;
}

static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group) {
    (void)plugin;
    (void)group;

    return 0;
}

static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag, neu_value_u value) {
    (void)plugin;
    (void)req;
    (void)tag;
    (void)value;

    return 0;
}