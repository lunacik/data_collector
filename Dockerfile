FROM ubuntu:bionic

RUN apt-get update && apt-get install -y build-essential cmake python3 python-pip

RUN pip install conan

WORKDIR /app/data_collector

COPY CMakeLists.txt ./
COPY src ./src
COPY cmake ./cmake

RUN mkdir build

WORKDIR build

RUN cmake ../ -DENABLE_TESTING=0 && make -j7

ENV TMP_PERSIST_DIR=/tmp/data_collector
ENV LOG_DIR_PATH=/var/lib/dc
ENV HOST=10.0.16.10
ENV PORT=8888

CMD mkdir -p $TMP_PERSIST_DIR && \
    mkdir -p $LOG_DIR_PATH && \
    ./bin/data_collector --tmp-persist-dir $TMP_PERSIST_DIR --host $HOST --port $PORT --log-dir=$LOG_DIR_PATH --dist-interval=10

