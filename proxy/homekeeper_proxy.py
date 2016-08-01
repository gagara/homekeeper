from functools import wraps

from elasticsearch import Elasticsearch
from flask import Flask
from flask import request, Response
import requests
from flask import jsonify


app = Flask(__name__)

es = Elasticsearch()
####
#app.debug = True
####

app.config.from_pyfile('proxy.conf')

####### Auth decorator #########
def check_auth(username, password):
    return username == app.config['AUTH_USER'] and password == app.config['AUTH_PASS']

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

def controller_send(req):
    url = req['m']
    if 'id' in req:
        url += '/%d' % req['id']
    if 'ns' in req:
        url += '/%d' % req['ns']
    if 'ft' in req:
        url += '/%d' % req['ft']
    host = "http://%s:%d/" % (app.config['CONTROLLER_HOST'], app.config['CONTROLLER_PORT'])
    if app.debug : print('>>>CONTROLLER: %s' % host + url)
    requests.get(host + url, timeout=app.config['CONTROLLER_CONN_TIMEOUT_SEC'])

def logserver_send(req):
    headers = {'User-Agent': 'proxy','Content-Type': 'application/x-www-form-urlencoded'}
    host = "http://%s:%d" % (app.config['LOGSERVER_HOST'], app.config['LOGSERVER_PORT'])
    if app.debug : print('>>>LOGSERVER: %s; %s' % (host, req))
    requests.post(host, json=req, headers=headers, timeout=app.config['LOGSERVER_CONN_TIMEOUT_SEC'])

def query_logs(timestamp):
    query = {"query": {"bool": {"filter": [{"term": {"headers.http_user_agent": "ESP8266"}}, {"range": {"@timestamp": {"gt": timestamp}}}]}}, "sort": {"@timestamp": "asc"}}
    logs = []
    if app.debug : print('>>>ELASTICSEARCH: %s' % query)
    res = es.search(index=app.config['ELASTICSEARCH_INDEX'], body=query)
    if app.debug : print('<<<ELASTICSEARCH: %d' % res['hits']['total'])
    for hit in res['hits']['hits']:
        if '@timestamp' in hit['_source'] and 'message' in hit['_source']:
            logs.append({'@timestamp': hit['_source']['@timestamp'], 'message': hit['_source']['message']})
    return logs


@app.route("/", methods=['POST'])
@requires_auth
def client_request():
    if app.debug : print('REQUEST: %s' % request.json)
    try:
        req = request.json
        if req['m'] == 'log':
            timestamp = req['ts']
            try:
                return jsonify(query_logs(timestamp)), 200
            except Exception as e:
                return (str(e), 504)
        else:
            try:
                controller_send(req)
                logserver_send(req)
                return jsonify({'result': 'OK'}), 200
            except Exception as e:
                return (str(e), 504)
    except Exception as e:
        return (str(e), 400)


if __name__ == "__main__":
    app.run(host='0.0.0.0')
