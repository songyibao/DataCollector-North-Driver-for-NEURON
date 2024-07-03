//
// Created by root on 7/3/24.
//

#ifndef COLLECTOR_UPDATE_INTERVAL_H
#define COLLECTOR_UPDATE_INTERVAL_H

void get_subscribed_group(const char *node, neu_plugin_t *plugin);

void get_node_group_interval(const char *node, const char *group, neu_plugin_t *plugin);


int update_interval(char *node, char *group, int interval, neu_plugin_t *plugin);

#endif //COLLECTOR_UPDATE_INTERVAL_H
