
aux_source_directory(buffer files)
aux_source_directory(config files)
aux_source_directory(http files)
aux_source_directory(log files)
aux_source_directory(pool files)
aux_source_directory(server files)
aux_source_directory(timer files)


add_library(webserver STATIC ${files})
target_link_libraries(webserver mysqlclient)