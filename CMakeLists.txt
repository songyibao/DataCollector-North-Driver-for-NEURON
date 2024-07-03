cmake_minimum_required(VERSION 3.13)
set(LIBRARY_OUTPUT_PATH "./")
file(COPY /software/neuron/plugins/data_collect/collector.json DESTINATION
        /software/neuron/build/plugins/schema)
project(collector)


add_library(plugin-collector SHARED collector.c
        service/service.c
        utils/utils.c
        file_writer/writer.c
        http_client/update_interval.c
)

target_include_directories(plugin-collector PRIVATE
        /software/neuron/include/neuron
)

target_link_libraries(plugin-collector PRIVATE z ev)
target_link_libraries(plugin-collector PRIVATE /usr/local/lib/neuron/libneuron-base.so curl /usr/local/lib/libyyjson.so)