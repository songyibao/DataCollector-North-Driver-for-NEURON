//
// Created by root on 5/23/24.
//
#include "writer.h"
int open_new_file(neu_plugin_t *plugin) {
    if (plugin->fp != NULL) {
        fclose(plugin->fp);
        plugin->fp = NULL;
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

uint64_t get_file_size(neu_plugin_t *plugin) {
    long current_pos = ftell(plugin->fp);
    fseek(plugin->fp, 0L, SEEK_END);
    long size = ftell(plugin->fp);
    fseek(plugin->fp, current_pos, SEEK_SET);  // 恢复文件指针位置
    return size;
}
// Function to add a string to the file
int add_string_to_file(neu_plugin_t *plugin, char *str)
{
    if (plugin->fp == NULL) {
        plog_error(plugin, "File pointer is NULL");
        return -1;
    }

    fprintf(plugin->fp, "%s\n", str);
    fflush(plugin->fp);
    // 如果文件大小超过了限制，就关闭当前文件，打开新文件
    if (get_file_size(plugin) > plugin->max_file_size) {
        open_new_file(plugin);
    }
    return 0;
}
