
set(sources main.cpp data_generator.cpp data_distributor.cpp output_file_handler.cpp http_transmitter.cpp)

set(headers data_generator.h data_distributor.h output_file_handler.h http_transmitter.h file_extensions.h)

#Generic exec that uses conan libs
add_executable(data_collector ${sources} ${headers})
target_link_libraries(data_collector PRIVATE project_options project_warnings
                            CONAN_PKG::docopt.cpp CONAN_PKG::fmt CONAN_PKG::spdlog CONAN_PKG::asio
                            CONAN_PKG::boost_system CONAN_PKG::boost_filesystem CONAN_PKG::boost_beast)
