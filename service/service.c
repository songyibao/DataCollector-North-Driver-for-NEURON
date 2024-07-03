//
// Created by root on 5/23/24.
//
#include "service.h"

#include "../file_writer/writer.h"
#include "../http_client/update_interval.h"
#include "../utils/utils.h"
#include "jansson.h"
#include "json/neu_json_rw.h"
#include "math.h"

void remove_chars(char *str) {
    int len = strlen(str);
    int i, j = 0;

    // 移除开头的[
    if (str[0] == '[') {
        for (i = 0; i < len - 1; i++) {
            str[i] = str[i + 1];
        }
        str[len - 1] = '\0';
        len--;
    }

    // 移除结尾的]
    if (str[len - 1] == ']') {
        str[len - 1] = '\0';
        len--;
    }

    // 移除字符串中的空格
    for (i = 0; i < len; i++) {
        if (str[i] != ' ') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';
}

int transform(neu_plugin_t *plugin, char *input_json_str, char **output_json_str) {
    json_t *input_root, *res_arr;
    json_error_t error;

    // 解析输入JSON字符串
    input_root = json_loads(input_json_str, 0, &error);

    if (!input_root) {
        plog_error(plugin, "error: on line %d: %s\n", error.line, error.text);
        return 1;
    }

    // 提取并重组数据
    res_arr = json_array();
    if (json_is_array(json_object_get(input_root, "tags"))) {
        size_t index;
        json_t *value;

        json_t *tags_array = json_object_get(input_root, "tags");
        size_t tags_size = json_array_size(tags_array);
        // 特殊需求时i从2开始，0为heart 1为start_flag
        for (index = 0; index < tags_size && (value = json_array_get(tags_array, index)); index++) {
            json_t *real_value = json_object_get(value, "value");

            // 根据plugin->precision来判断保留几位小数,plugin->precision=100时，保留两位小数
            json_real_set(real_value, (round(json_real_value(real_value) * plugin->precision)) /
                                          plugin->precision);

            if (json_is_real(real_value) || json_is_integer(real_value)) {
                json_array_append_new(res_arr, real_value);
            }
        }
        // 预留位补零，特殊需求
        //        for (int i = 0; i < 7; i++) {
        //            json_t *real_value = json_real(0);
        //            json_array_append_new(res_arr, real_value);
        //        }
        if (plugin->with_timestamp) {
            // 配置需要时间戳
            json_t *integer_value = json_integer(neu_time_ms());

            if (json_is_integer(integer_value)) {
                json_array_append_new(res_arr, integer_value);
            }
        }
    }

    char *tmp = json_dumps(res_arr, JSON_INDENT(0) | JSON_PRESERVE_ORDER);
    remove_chars(tmp);
    *output_json_str = strdup(tmp);
    free(tmp);
    // 释放内存
    json_decref(input_root);
    json_decref(res_arr);

    return 0;
}

int handle_trans_data(neu_plugin_t *plugin, neu_reqresp_head_t *head,
                      neu_reqresp_trans_data_t *trans_data) {
    int ret = 0;
    char *json_str = NULL;
    char *transformed_str = NULL;
    neu_json_read_resp_t resp = {0};

    tag_ut_array_to_neu_json_read_resp_t(trans_data->tags, &resp);
    for (int i = 0; i < resp.n_tag; i++) {
        if (resp.tags[i].error != 0) {
            plog_error(plugin, "tag %s error: %ld", resp.tags[i].name, resp.tags[i].error);
            return -1;
        }
    }
    ret = neu_json_encode_by_fn(&resp, neu_json_encode_read_resp, &json_str);
    if (ret != 0) {
        plog_notice(plugin, "parse json failed");
        return -1;
    }
    //    plog_debug(plugin, "parse json str succeed: %s", json_str);
    if (plugin->pre_str != NULL) {
        if (strcmp(plugin->pre_str, json_str) == 0) {
            if (plugin->need_dynamic_interval == true) {
                plog_debug(plugin, "数据重复,需要增大采集间隔");
                // 如果成功，函数内部会更新plugin->node_group_interval的值
                update_interval(plugin->node_name, plugin->group_name,
                                plugin->node_group_interval + 1, plugin);
            }
            if (plugin->need_deduplication == true) {
                plog_debug(plugin, "数据重复,需要去重,不记录");
                // 与上一条数据相同，需要去重，不处理
                return 0;
            }
        } else {
            if (plugin->need_dynamic_interval == true) {
                plog_debug(plugin, "数据不重复,需要减小采集间隔");
                // 如果成功，函数内部会更新plugin->node_group_interval的值
                update_interval(plugin->node_name, plugin->group_name,
                                plugin->node_group_interval - 5, plugin);
            }
            free(plugin->pre_str);
            plugin->pre_str = strdup(json_str);
        }
    } else {
        plugin->pre_str = strdup(json_str);
    }

    int res = transform(plugin, json_str, &transformed_str);
    free(json_str);
    if (res != 0) {
        plog_error(plugin, "transform json failed");

        return -1;
    }
    plog_debug(plugin, "transform json str succeed: %s", transformed_str);

    res = add_string_to_file(plugin, transformed_str);
    free(transformed_str);
    if (res != 0) {
        plog_error(plugin, "add string to file failed");
        return -1;
    }
    return 0;
}