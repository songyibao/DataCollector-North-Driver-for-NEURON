//
// Created by root on 5/23/24.
//

#ifndef NEURON_COLLECTOR_H
#define NEURON_COLLECTOR_H
#include "neuron.h"
struct neu_plugin {
    neu_plugin_common_t common;

    // 可选参数
    // 小数保留位数
    int precision;
    // 每条记录是否添加时间戳
    bool with_timestamp;
    // 最大文件大小,单位MB
    uint64_t max_file_size;
    // 文件路径
    char path[255];
    // 文件指针
    FILE *fp;
    // 插件启动状态
    bool started;
};
static neu_plugin_t *driver_open(void);

static int driver_close(neu_plugin_t *plugin);
static int driver_init(neu_plugin_t *plugin, bool load);
static int driver_uninit(neu_plugin_t *plugin);
static int driver_start(neu_plugin_t *plugin);
static int driver_stop(neu_plugin_t *plugin);
static int driver_config(neu_plugin_t *plugin, const char *config);
static int driver_request(neu_plugin_t *plugin, neu_reqresp_head_t *head,
                          void *data);

static int driver_validate_tag(neu_plugin_t *plugin, neu_datatag_t *tag);
static int driver_group_timer(neu_plugin_t *plugin, neu_plugin_group_t *group);
static int driver_write(neu_plugin_t *plugin, void *req, neu_datatag_t *tag,
                        neu_value_u value);
#endif // NEURON_COLLECTOR_H
