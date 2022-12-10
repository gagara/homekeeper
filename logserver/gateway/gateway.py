from calendar import timegm
from datetime import datetime
from functools import wraps
import json
import os
from time import time

from elasticsearch import Elasticsearch
from flask import Flask, request
from flask.wrappers import Response
from flask_restx import Api, Resource
import requests

ES_DATE_FORMAT = "%Y-%m-%dT%H:%M:%S.%fZ"

app = Flask(__name__)
app.debug = True
api = Api(app, version='1.0', title='Homekeeper API Server',
          description='API server for homekeeper backend')

nodes = api.namespace('control', description='Node controlling API')
sensors = api.namespace('config', description='Sensor configuration API')
deviceapi = api.namespace('device', description='Device Raw API')

# read config file
app.config.from_pyfile('gateway.conf')

# read env
app.config['GATEWAY']['host'] = os.environ.get('GATEWAY_HOST', default=app.config['GATEWAY']['host'])
app.config['GATEWAY']['port'] = int(os.environ.get('GATEWAY_PORT', default=app.config['GATEWAY']['port']))
app.config['GATEWAY']['user'] = os.environ.get('GATEWAY_USER', default=app.config['GATEWAY']['user'])
app.config['GATEWAY']['password'] = os.environ.get('GATEWAY_PASSWORD', default=app.config['GATEWAY']['password'])
app.config['CONTROLLERS']['DAD']['host'] = os.environ.get('DAD_HOST', default=app.config['CONTROLLERS']['DAD']['host'])
app.config['CONTROLLERS']['DAD']['port'] = int(os.environ.get('DAD_PORT', default=app.config['CONTROLLERS']['DAD']['port']))
app.config['CONTROLLERS']['MOM']['host'] = os.environ.get('MOM_HOST', default=app.config['CONTROLLERS']['MOM']['host'])
app.config['CONTROLLERS']['MOM']['port'] = int(os.environ.get('MOM_PORT', default=app.config['CONTROLLERS']['MOM']['port']))
app.config['ELASTICSEARCH']['host'] = os.environ.get('ELASTICSEARCH_HOST', default=app.config['ELASTICSEARCH']['host'])
app.config['ELASTICSEARCH']['port'] = int(os.environ.get('ELASTICSEARCH_PORT', default=app.config['ELASTICSEARCH']['port']))
app.config['ELASTICSEARCH']['index'] = os.environ.get('ELASTICSEARCH_INDEX', default=app.config['ELASTICSEARCH']['index'])
app.config['LOGSERVER']['host'] = os.environ.get('LOGSERVER_HOST', default=app.config['LOGSERVER']['host'])
app.config['LOGSERVER']['port'] = int(os.environ.get('LOGSERVER_PORT', default=app.config['LOGSERVER']['port']))

es = Elasticsearch("http://" + app.config['ELASTICSEARCH']['host'] + ":" + str(app.config['ELASTICSEARCH']['port']))

ccd = {
    app.config['CONTROLLERS']['DAD']['host']: {'timestamp': 0, 'value': 0},
    app.config['CONTROLLERS']['MOM']['host']: {'timestamp': 0, 'value': 0}
}

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
        if app.config['GATEWAY']['user']:
            auth = request.authorization
            if not auth or not check_auth(auth.username, auth.password):
                return authenticate()
        return f(*args, **kwargs)
    return decorated
####### #### ######### #########

def controller_send(conf, body):
    if conf:
        headers = {'User-Agent': 'gw', 'Content-Type': 'application/json', 'Connection': 'close'}
        host = "http://%s:%d" % (conf['host'], conf['port'])
        retry = 0
        rcode = 0
        while retry < conf['conn_max_retry'] and rcode != 200:
            if app.debug : print('>>>CONTROLLER: %s; %s' % (host, body))
            try:
                r = requests.post(host, json=body, headers=headers, timeout=conf['conn_timeout_sec'])
                rcode = r.status_code
            except:
                rcode = 0
            retry = retry + 1
        if rcode != 200:
            raise Exception("unable to contact controller")
    else:
        raise ValueError("unknown controller")

def logserver_send(body):
    headers = {'User-Agent': 'gw', 'Content-Type': 'application/json'}
    host = "http://%s:%d" % (app.config['LOGSERVER']['host'], app.config['LOGSERVER']['port'])
    if app.debug : print('>>>LOGSERVER: %s; %s' % (host, body))
    requests.post(host, json=body, headers=headers, timeout=app.config['LOGSERVER']['conn_timeout_sec'])

def query_es(query):
    logs = []
    if app.debug : print('>>>ELASTICSEARCH: %s' % query)
    res = es.search(index=app.config['ELASTICSEARCH']['index'], body=query)
    for hit in res['hits']['hits']:
        if '@timestamp' in hit['_source'] and 'host' in hit['_source'] and 'message' in hit['_source']:
            logs.append({'@timestamp': timegm(datetime.strptime(hit['_source']['@timestamp'], ES_DATE_FORMAT).timetuple()),
                         'host': hit['_source']['host'],
                         'message': json.loads(hit['_source']['message'])})
    if app.debug : print('<<<ELASTICSEARCH: %d/%d' % (len(logs), res['hits']['total']))
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
        # if current 'cls' is too old (>60m) || requested long period (>=3m) then trigger 'cls' request
        if (time() - ccd[conf['host']]['timestamp']) > (60 * 60) or (time() - req_ts) >= (3 * 60):
            controller_send(conf, {'m': 'cls'})
    except Exception as e:
        if app.debug : print(e)

def identify_target_controller(msg):
    if 'id' in msg:
        if msg['id'] in app.config['CONTROLLERS']['DAD']['managed_nodes']:
            return app.config['CONTROLLERS']['DAD']
        elif msg['id'] in app.config['CONTROLLERS']['MOM']['managed_nodes']:
            return app.config['CONTROLLERS']['MOM']
        else:
            return None
    elif 'n' in msg and 'id' in msg['n']:
        if msg['n']['id'] in app.config['CONTROLLERS']['DAD']['managed_nodes']:
            return app.config['CONTROLLERS']['DAD']
        elif msg['n']['id'] in app.config['CONTROLLERS']['MOM']['managed_nodes']:
            return app.config['CONTROLLERS']['MOM']
        else:
            return None
    elif 's' in msg and 'id' in msg['s']:
        if msg['s']['id'] in app.config['CONTROLLERS']['DAD']['managed_nodes']:
            return app.config['CONTROLLERS']['DAD']
        elif msg['s']['id'] in app.config['CONTROLLERS']['MOM']['managed_nodes']:
            return app.config['CONTROLLERS']['MOM']
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
            if ccd[log['host']] and ccd[log['host']]['timestamp'] > 0:
                delta = ccd[log['host']]['value']
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
    init_controller_clock_delta(app.config['CONTROLLERS']['DAD'], timestamp)
    init_controller_clock_delta(app.config['CONTROLLERS']['MOM'], timestamp)

    return process_logs(query_es({"query": {"bool": {"filter": [
        {"term": {"headers.http_user_agent": "ESP8266"}},
        {"range": {"@timestamp": {"gt": timestamp * 1000}}}
        ]}}, "sort": {"@timestamp": "asc"}, "size": 50}))


############## Swagger API ##############

node_upd_model = nodes.parser()
node_upd_model.add_argument(
    'node', type=str, help='Node', required=True, choices=list(app.config['NODES'].keys()), location='form')
node_upd_model.add_argument(
    'mode', type=str, help='Mode', required=True, choices=['manual', 'auto'], default='manual', location='form')
node_upd_model.add_argument(
    'state', type=str, help='State', required=True, choices=['ON', 'OFF'], default='ON', location='form')
node_upd_model.add_argument(
    'period', type=int, help='Period (minutes) [0 - forever]', required=True, default=5, location='form')

node_get_model = nodes.parser()
node_get_model.add_argument(
    'node', type=str, help='Node', required=True, choices=list(app.config['NODES'].keys()))

@nodes.route("")
class NodeControlApi(Resource):

    @requires_auth
    @api.doc(description='Change node state', parser=node_upd_model)
    def post(self):
        args = node_upd_model.parse_args()
        if app.debug : print('REQUEST: %s' % args)
        req = {}
        req['m'] = 'nsc'
        req['id'] = app.config['NODES'][args['node']]
        if args['mode'] == "manual":
            req['ns'] = 1 if args['state'] == "ON" else 0
            if args['period'] > 0:
                req['ft'] = args['period'] * 60
        try:
            controller_send(identify_target_controller(req), req)
            logserver_send(req)
            return {"success": "200 OK"}, 200
        except ValueError as e:
            if app.debug : print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug : print(e)
            return {"error": str(e)}, 500

    @requires_auth
    @api.doc(description='Query node state', parser=node_get_model)
    def get(self):
        if app.debug: print('REQUEST: %s' % request.args)
        try:
            id = app.config['NODES'][request.args['node']]
            res = query_es({"query": {"bool": {"filter": [
                {"term": {"headers.http_user_agent": "ESP8266"}},
                {"term": {"m": "csr"}},
                {"term": {"id": id}},
            ]}}, "sort": {"@timestamp": "desc"}, "size": 1})
            response = {}
            if len(res) > 0 and res[0]['message'] and res[0]['@timestamp']:
                response['state'] = \
                    'ON' if res[0]['message']['n']['ns'] == 1 else \
                    'OFF' if res[0]['message']['n']['ns'] == 0 else \
                    'ERR'
                response['timestamp'] = datetime.fromtimestamp(res[0]['@timestamp']).strftime('%Y-%m-%d %H:%M:%S')
                response['age'] = str((datetime.now() - datetime.fromtimestamp(res[0]['@timestamp'])).seconds) + " seconds"
            return response, 200
        except ValueError as e:
            if app.debug:
                print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug:
                print(e)
            return {"error": str(e)}, 500

######

sensors_cfg_model = sensors.parser()
sensors_cfg_model.add_argument(
    'sensor', type=str, help='Sensor', required=True, choices=list(app.config['SENSORS'].keys()), location='form')
sensors_cfg_model.add_argument(
    'value', type=int, help='Value (\u2103)', required=True, default=20, location='form')

sensors_get_model = sensors.parser()
sensors_get_model.add_argument(
    'sensor', type=str, help='Sensor', required=True, choices=list(app.config['SENSORS'].keys()))

@sensors.route("")
class SensorControlApi(Resource):

    @requires_auth
    @api.doc(description='Configure sensor thresholds', parser=sensors_cfg_model)
    def post(self):
        args = sensors_cfg_model.parse_args()
        if app.debug : print('REQUEST: %s' % args)
        req = {}
        req['m'] = 'cfg'
        req['s'] = {}
        req['s']['id'] = app.config['SENSORS'][args['sensor']]
        req['s']['v'] = args['value']
        try:
            controller_send(identify_target_controller(req), req)
            logserver_send(req)
            return {"success": "200 OK"}, 200
        except ValueError as e:
            if app.debug : print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug : print(e)
            return {"error": str(e)}, 500

    @requires_auth
    @api.doc(description='Query sensor thresholds', parser=sensors_get_model)
    def get(self):
        if app.debug: print('REQUEST: %s' % request.args)
        try:
            id = app.config['SENSORS'][request.args['sensor']]
            res = query_es({"query": {"bool": {"filter": [
                {"term": {"headers.http_user_agent": "ESP8266"}},
                {"term": {"m": "cfg"}},
                {"term": {"s.id": id}},
            ]}}, "sort": {"@timestamp": "desc"}, "size": 1})
            response = {}
            if len(res) > 0 and res[0]['message'] and res[0]['@timestamp']:
                response['value'] = res[0]['message']['s']['v']
                response['timestamp'] = datetime.fromtimestamp(res[0]['@timestamp']).strftime('%Y-%m-%d %H:%M:%S')
                response['age'] = str((datetime.now() - datetime.fromtimestamp(res[0]['@timestamp'])).seconds) + " seconds"
            return response, 200
        except ValueError as e:
            if app.debug:
                print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug:
                print(e)
            return {"error": str(e)}, 500
######

deviceapi_model = deviceapi.parser()
deviceapi_model.add_argument(
    'name', type=str, help='Controller', required=True, choices=list(app.config['CONTROLLERS'].keys()))
deviceapi_model.add_argument(
    'message', type=str, help='JSON message', required=True, location='json')

@deviceapi.route("/<name>")
@api.doc(description='Send command to controller', parser=deviceapi_model)
class ControllerRawApi(Resource):

    @requires_auth
    def post(self, name):
        if app.debug : print('REQUEST: dev: %s; msg: %s' % (name, request.data))
        try:
            cfg = app.config['CONTROLLERS'][name] if name in app.config['CONTROLLERS'] else None
            msg = json.loads(request.data)
            controller_send(cfg, msg)
            return {"success": "200 OK"}, 200
        except ValueError as e:
            if app.debug : print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug : print(e)
            return {"error": str(e)}, 500

######

app_model = api.model('json', {})

@api.route("/", doc=False)
@api.doc(description='API used by Application', parser=app_model)
class ApplicationApi(Resource):

    @requires_auth
    def post(self):
        if app.debug : print('REQUEST: %s' % request.data)
        try:
            body = json.loads(request.data)
            if 'm' in body and body['m'] == 'log':
                timestamp = body['ts'] if 'ts' in body else 0
                return query_logs(timestamp), 200
            else:
                controller_send(identify_target_controller(body), body)
                logserver_send(body)
                return {"success": "200 OK"}, 200
        except ValueError as e:
            if app.debug : print(e)
            return {"error": str(e)}, 400
        except Exception as e:
            if app.debug : print(e)
            return {"error": str(e)}, 500


if __name__ == "__main__":
    context = ('.ssl/server.crt', '.ssl/server.key')
    app.run(host=app.config['GATEWAY']['host'], port=app.config['GATEWAY']['port'], ssl_context=context)

