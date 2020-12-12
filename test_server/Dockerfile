FROM alpine

RUN apk add python3 py3-pip

RUN pip install flask

WORKDIR /app/dc_server

COPY server.py .

ENV LOG_DIR_PATH=/var/lib/dc/logs
ENV HOST=0.0.0.0
ENV PORT=8888

EXPOSE $PORT

CMD mkdir -p $LOG_DIR_PATH && python3 -u server.py 2>&1 | tee -a $LOG_DIR_PATH/dc_server.log

