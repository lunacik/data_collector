# Data collection and distribution
### Abstract
Purpose of this project is to simulate a device which at random period writes data to the file while some other service collects this data and transmits it over http to the server. Data generating service must group data into files. Data collecting service must distribute available data every one minute. Files containing data that was already distributed shall be removed from the filesystem.

### Solution
Collection and generation services will run as a separate threads on the same process.
Data grouping in files is done in accordance to the number of elapsed minutes from the start of unix time epoch. For example: data generated at 07/24/2020 @ 10:56am (UTC) would be put into file named `26593136.dat`. In order to prevent any race conditions and synchronizations issues an explicit file locking is performed. As long as data generator is writing to the `26593136.dat` there will be always a file named `26593136.dat.lock` which serves a lock and an indication for the distribution process that `26593136.dat` is not yet available for reading. Data generation service guarantees that it will remove the lock as soon as it stops writing to the file.

### Prerequisities for manually building and running the project
Project template copied and stripped down from ['cpp_starter_project'](https://github.com/lefticus/cpp_starter_project).
Tools required to build the project manually: **C++14 compiler**, **cmake**, **conan** (python).

#### Building
Create 'build' directory in project's root folder, cd to `build` and run `cmake ..`. Build process produces `bin/data_collector` executable file.

#### Running
##### Usage:
#
          data_collector --tmp-persist-dir=<path> --host=<host> --port=<port> [--dist-interval=<sec>] [--log-dir=<path>]
          data_collector (-h | --help)

    Options:
          -h --help                 Show this screen.
          --tmp-persist-dir=<path>  Path to directory where data will be temporarily stored.
          --host=<host>             Name or ip address of host server.
          --port=<port>             Port of host server.
          --dist-interval=<sec>     Data distribution to server interval in seconds [default: 60].
          --log-dir=<path>          Optional path to a directory where all logs will be written with rotation interval of 7days [default: ].
##### Example
For example it can be started with the following arguments (to stop use CTRL+C):

    $ ./bin/data_collector --tmp-persist-dir /tmp/data --host localhost --port 8888 --log-dir=/var/log --dist-interval=10
    
#### External libraries
* spdlog - for logging events
* docopt - convienient arguments parsing
* fmt - convienient text formatting
* boost.asio - timer events and tcp sockets
* boost.filesystem - filesystem access
* boost.beast - dispatching http requests

#### Plaform compatibility
Application was tested on CentOS 7.3 and Ubuntu 18.04 but complete source code should be platform independent.

### Running test server
`test_server` folder contains python3 script which can be executed to start an http server which accepts post requests on `/` target. It will simply print out the received payload. In order to use you must first install `flask` framework. Following command starts server on localhost port 1080.

    $  ./run.sh --port 8888 --host localhost
    or
    $ PORT=8888 HOST=localhost ./server.py
    
### Building and running with Docker
Project root folder and `test_server` contains Dockerfile which can be used to build docker image manually, alternatively docker-compose can be used to build and start test server and a client with the following commands:

    $ docker-compose build
    $ docker-compose up
    
### Additional implementation notes
Current implementation performs blocking IO for both file system access and http requests, this is a subject for further improvements.
Parameter `--dist-interval` is not very meaningful because files containing data are created every 1min, this concept probably has to be reconsidered.

