from functools import wraps

import os

from elasticsearch import Elasticsearch
import requests
import json

from flask import Flask
from flask import jsonify
from flask import request, Response

from time import time
from calendar import timegm
import datetime

ES_DATE_FORMAT = "%Y-%m-%dT%H:%M:%S.%fZ"

app = Flask(__name__)

# read config file
app.config.from_pyfile('gateway.conf')

# read env
app.config['GATEWAY']['host'] = os.environ.get('GATEWAY_HOST', default=app.config['GATEWAY']['host'])
app.config['GATEWAY']['port'] = os.environ.get('GATEWAY_PORT', default=app.config['GATEWAY']['port'])
app.config['GATEWAY']['user'] = os.environ.get('GATEWAY_USER', default=app.config['GATEWAY']['user'])
app.config['GATEWAY']['password'] = os.environ.get('GATEWAY_PASSWORD', default=app.config['GATEWAY']['password'])
app.config['DAD']['host'] = os.environ.get('DAD_HOST', default=app.config['DAD']['host'])
app.config['DAD']['port'] = os.environ.get('DAD_PORT', default=app.config['DAD']['port'])
app.config['MOM']['host'] = os.environ.get('MOM_HOST', default=app.config['MOM']['host'])
app.config['MOM']['port'] = os.environ.get('MOM_PORT', default=app.config['MOM']['port'])
app.config['ELASTICSEARCH']['host'] = os.environ.get('ELASTICSEARCH_HOST', default=app.config['ELASTICSEARCH']['host'])
app.config['ELASTICSEARCH']['port'] = os.environ.get('ELASTICSEARCH_PORT', default=app.config['ELASTICSEARCH']['port'])
app.config['ELASTICSEARCH']['index'] = os.environ.get('ELASTICSEARCH_INDEX', default=app.config['ELASTICSEARCH']['index'])
app.config['LOGSERVER']['host'] = os.environ.get('LOGSERVER_HOST', default=app.config['LOGSERVER']['host'])
app.config['LOGSERVER']['port'] = os.environ.get('LOGSERVER_PORT', default=app.config['LOGSERVER']['port'])

####
app.debug = True
####

es = Elasticsearch(hosts=[app.config['ELASTICSEARCH']['host']], port=app.config['ELASTICSEARCH']['port'])

ccd = {app.config['DAD']['host']: {'timestamp': 0, 'value': 0}, app.config['MOM']['host']: {'timestamp': 0, 'value': 0}}

####### Auth decorator #########
def check_auth(username, password):
    return username == app.config['GATEWAY']['user'] and password == app.config['GATEWAY']['password']

def authenticate():
    return Response(
    '401 - Authentication Required', 401,
    {'WWW-Authenticate': 'Basic realm="Authentication Required"'})

def requires_auth(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        auth = request.authorization
        if not auth or not check_auth(auth.username, auth.password):
            return authenticate()
        return f(*args, **kwargs)
    return decorated
####### #### ######### #########

def controller_send(conf, req):
    if conf:
        headers = {'User-Agent': 'gw', 'Content-Type': 'application/json', 'Connection': 'close'}
        host = "http://%s:%d" % (conf['host'], conf['port'])
        retry = 0
        rcode = 0
        while retry < conf['conn_max_retry'] and rcode != 200:
            if app.debug : print('>>>CONTROLLER: %s; %s' % (host, req))
            try:
                r = requests.post(host, json=req, headers=headers, timeout=conf['conn_timeout_sec'])
                rcode = r.status_code
            except:
                rcode = 0
            retry = retry + 1
        if rcode != 200:
            raise Exception()

def logserver_send(req):
    headers = {'User-Agent': 'gw', 'Content-Type': 'application/json'}
    host = "http://%s:%d" % (app.config['LOGSERVER']['host'], app.config['LOGSERVER']['port'])
    if app.debug : print('>>>LOGSERVER: %s; %s' % (host, req))
    requests.post(host, json=req, headers=headers, timeout=app.config['LOGSERVER']['conn_timeout_sec'])

def query_es(query):
    logs = []
    if app.debug : print('>>>ELASTICSEARCH: %s' % query)
    res = es.search(index=app.config['ELASTICSEARCH']['index'], body=query)
    if app.debug : print('<<<ELASTICSEARCH: %d' % res['hits']['total'])
    for hit in res['hits']['hits']:
        if '@timestamp' in hit['_source'] and 'host' in hit['_source'] and 'message' in hit['_source']:
            logs.append({'@timestamp': timegm(datetime.datetime.strptime(hit['_source']['@timestamp'], ES_DATE_FORMAT).timetuple()),
                         'host': hit['_source']['host'],
                         'message': json.loads(hit['_source']['message'])})
    return logs

def init_controller_clock_delta(conf, req_ts):
    try:
        if (time() - ccd[conf['host']]['timestamp']) > (60 * 60):
            cls = query_es({"query": {"bool": {"filter": [
                {"term": {"headers.http_user_agent": "ESP8266"}},
                {"term": {"host": conf['host']}},
                {"term": {"m": "cls"}}
                ]}}, "sort": {"@timestamp": "desc"}, "size": 1})
            if len(cls) > 0 and 'ts' in cls[0]['message']:
                ccd[conf['host']]['value'] = cls[0]['@timestamp'] - cls[0]['message']['ts']
                ccd[conf['host']]['timestamp'] = cls[0]['@timestamp']
        # if current 'cls' is too old (>60m) || requested long period (>5m) then trigger 'cls' request
        if (time() - ccd[conf['host']]['timestamp']) > (60 * 60) or (time() - req_ts) >= (5 * 60):
            controller_send(conf, {'m': 'cls'})
    except Exception as e:
        if app.debug : print(e)

def identify_target_controller(msg):
    if 'id' in msg:
        if msg['id'] in app.config['DAD']['managed_nodes']:
            return app.config['DAD']
        elif msg['id'] in app.config['MOM']['managed_nodes']:
            return app.config['MOM']
        else:
            return None
    elif 'n' in msg and 'id' in msg['n']:
        if msg['n']['id'] in app.config['DAD']['managed_nodes']:
            return app.config['DAD']
        elif msg['n']['id'] in app.config['MOM']['managed_nodes']:
            return app.config['MOM']
        else:
            return None
    elif 's' in msg and 'id' in msg['s']:
        if msg['s']['id'] in app.config['DAD']['managed_nodes']:
            return app.config['DAD']
        elif msg['s']['id'] in app.config['MOM']['managed_nodes']:
            return app.config['MOM']
        else:
            return None
    else:
        return None

def process_logs(logs):
    result = []
    for log in logs:
        if 'm' in log['message']:
            # update ccd if 'cls' message received
            if log['message']['m'] == 'cls' and 'ts' in log['message'] and log['host'] in ccd and log['@timestamp'] > ccd[log['host']]['timestamp']:
                ccd[log['host']]['timestamp'] = log['@timestamp']
                ccd[log['host']]['value'] = log['@timestamp'] - log['message']['ts']
            # update timesatmps
            delta = None
            if ccd.get(log['host']) and ccd.get(log['host'])['timestamp'] > 0:
                delta = ccd.get(log['host'])['value']
            if 'ts' in log['message'] and int(log['message']['ts']) > 0 :
                log['message']['ts'] = int(int(log['message']['ts']) + delta) if delta else 0
            if 'ft' in log['message'] and int(log['message']['ft']) > 0 :
                log['message']['ft'] = int(int(log['message']['ft']) + delta) if delta else 0
            if 'n' in log['message'] and 'ts' in log['message']['n'] and int(log['message']['n']['ts']) > 0 :
                log['message']['n']['ts'] = int(int(log['message']['n']['ts']) + delta) if delta else 0
            if 'n' in log['message'] and 'ft' in log['message']['n'] and int(log['message']['n']['ft']) > 0 :
                log['message']['n']['ft'] = int(int(log['message']['n']['ft']) + delta) if delta else 0
            if 's' in log['message'] and 'ts' in log['message']['s'] and int(log['message']['s']['ts']) > 0 :
                log['message']['s']['ts'] = int(int(log['message']['s']['ts']) + delta) if delta else 0
            if 's' in log['message'] and 'ft' in log['message']['s'] and int(log['message']['s']['ft']) > 0 :
                log['message']['s']['ft'] = int(int(log['message']['s']['ft']) + delta) if delta else 0
            log.pop('host', None)
            if log['message']['m'] in ['csr', 'nsc', 'cfg']:
                result.append(log)
    return result

def query_logs(timestamp):
    init_controller_clock_delta(app.config['DAD'], timestamp)
    init_controller_clock_delta(app.config['MOM'], timestamp)

    return process_logs(query_es({"query": {"bool": {"filter": [
        {"term": {"headers.http_user_agent": "ESP8266"}},
        {"range": {"@timestamp": {"gt": timestamp * 1000}}}
        ]}}, "sort": {"@timestamp": "asc"}}))

@app.route("/", methods=['POST'])
@requires_auth
def client_request():
    if app.debug : print('REQUEST: %s' % request.json)
    try:
        req = request.json
        if req['m'] == 'log':
            timestamp = req['ts']
            try:
                return (jsonify(query_logs(timestamp)), 200)
            except Exception as e:
                if app.debug : print(e)
                return (str(e), 500)
        else:
            try:
                controller_send(identify_target_controller(req), req)
                logserver_send(req)
                return (jsonify([]), 200)
            except Exception as e:
                if app.debug : print(e)
                return (str(e), 500)
    except Exception as e:
        if app.debug : print(e)
        return (str(e), 400)


if __name__ == "__main__":
    context = ('.ssl/server.crt', '.ssl/server.key')
    app.run(host=app.config['GATEWAY']['host'], port=app.config['GATEWAY']['port'], ssl_context=context)

