#!/usr/bin/python3

from flask import Flask
from flask import request
import os

app = Flask(__name__)

@app.route('/', methods = ['POST'])
def receive_data():
    if request.method == 'POST':
        print("POST request received with following payload: \n" + request.data.decode('UTF-8'))
        return "Data successfully received"
    else:
        return "Unsupported method type"


if __name__ == '__main__':
      app.run(host='0.0.0.0', port=os.environ['PORT'])
