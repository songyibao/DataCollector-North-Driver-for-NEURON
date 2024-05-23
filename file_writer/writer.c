//
// Created by root on 5/23/24.
//
#include "writer.h"

// Function to add a string to the file
int add_string_to_file(neu_plugin_t *plugin, char *str)
{
    if (plugin->fp == NULL) {
        plog_error(plugin, "File pointer is NULL");
        return -1;
    }
    fprintf(plugin->fp, "%s\n", str);
    fflush(plugin->fp);
    return 0;
}
