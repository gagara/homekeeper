from flask import Flask, request, abort
from flask import jsonify
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy.exc import IntegrityError, OperationalError
from sqlalchemy.orm.exc import UnmappedInstanceError

app = Flask(__name__)

####
app.debug = True
####

#app.config['SQLALCHEMY_DATABASE_URI'] = 'mysql://slg:slg@localhost/hk'
app.config.from_envvar('HK_SETTINGS')
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

db = SQLAlchemy(app)

################## Node ##################
@app.route("/node", methods=['POST'])
def add_node():
    try:
        host = Host.query.filter_by(name=request.json.get('host')).one()
        pin = request.json.get('id')
        name = request.json.get('name')
        node = Node(host.id, pin, name)
        db.session.add(node)
        db.session.commit()
        return "", 201
    except IntegrityError:
        abort(409)
    except:
        abort(400)
 
@app.route("/node/<int:pin>", methods=['DELETE'])
def remove_node(pin):
    try:
        node = Node.query.filter_by(pin=pin).first()
        db.session.delete(node)
        db.session.commit()
        return "", 200
    except UnmappedInstanceError:
        abort(404)
    except:
        raise

@app.route("/node/<host_name>/<int:pin>", methods=['PUT'])
def update_node(host_name, pin):
    try:
        host = Host.query.filter_by(name=host_name).first()
        node = Node.query.filter_by(pin=pin, host_id=host.id).first()
        node.name = request.json.get('name') if request.json.has_key('name') else node.name
        db.session.commit()
        return "", 200
    except AttributeError:
        abort(404)
    except:
        raise

@app.route("/node/<host>/<int:pin>", methods=['GET'])
def get_node(host, pin):
    try:
        n = Node.query.filter_by(host=host, pin=pin).first()
        return jsonify({"m": "cfg", "n": [{"host": n.host.name, "id": n.pin, "name": n.name}]})
    except AttributeError:
        abort(404)

@app.route("/nodes", methods=['GET'])
def get_all_nodes():
    sql = "select h.name h_name, n.pin pin, n.name n_name from node n, host h where h.id=n.host_id;"
    return jsonify({"m": "cfg", "n": map(lambda n: ({"host": n['h_name'], "id": n['pin'], "name": n['n_name']}), db.engine.execute(sql))})

################## Sensor ##################
@app.route("/sensor", methods=['POST'])
def add_sensor():
    try:
        pin = request.json.get('id')
        name = request.json.get('name')
        sensor = Sensor(pin, name)
        db.session.add(sensor)
        db.session.commit()
        return "", 201
    except IntegrityError:
        abort(409)
    except:
        abort(400)
 
@app.route("/sensor/<int:pin>", methods=['DELETE'])
def remove_sensor(pin):
    try:
        sensor = Sensor.query.filter_by(pin=pin).first()
        db.session.delete(sensor)
        db.session.commit()
        return "", 200
    except UnmappedInstanceError:
        abort(404)
    except:
        raise

@app.route("/sensor/<int:pin>", methods=['PUT'])
def update_sensor(pin):
    try:
        sensor = Sensor.query.filter_by(pin=pin).first()
        sensor.name = request.json.get('name')
        db.session.commit()
        return "", 200
    except AttributeError:
        abort(404)
    except:
        raise

@app.route("/sensor/<int:pin>", methods=['GET'])
def get_sensor(pin):
    try:
        s = Sensor.query.filter_by(pin=pin).first()
        return jsonify({"m": "cfg", "s": [{"id": s.pin, "name": s.name}]})
    except AttributeError:
        abort(404)

@app.route("/sensors", methods=['GET'])
def get_all_sensors():
    return jsonify({"m": "cfg", "s": map(lambda s: ({"id": s.pin, "name": s.name}), Sensor.query.all())})

################## Host ##################
@app.route("/host", methods=['POST'])
def add_host():
    try:
        name = request.json.get('name')
        ip = request.json.get('ip')
        mac = request.json.get('mac')
        _type = request.json.get('type')
        host = Host(name, ip, mac, _type)
        db.session.add(host)
        db.session.commit()
        return "", 201
    except IntegrityError:
        abort(409)
    except OperationalError:
        abort(400)
 
@app.route("/host/<int:_id>", methods=['DELETE'])
def remove_host(_id):
    try:
        host = Host.query.get(_id)
        db.session.delete(host)
        db.session.commit()
        return "", 200
    except UnmappedInstanceError:
        abort(404)
    except:
        raise

@app.route("/host/<int:_id>", methods=['PUT'])
def update_host(_id):
    try:
        host = Host.query.get(_id)
        host.name = request.json.get('name') if request.json.has_key('name') else host.name
        host.ip = request.json.get('ip') if request.json.has_key('ip') else host.ip
        host.mac = request.json.get('mac') if request.json.has_key('mac') else host.mac
        host.type = request.json.get('type') if request.json.has_key('type') else host.type
        db.session.commit()
        return "", 200
    except AttributeError:
        abort(404)
    except:
        raise

@app.route("/host/<int:_id>", methods=['GET'])
def get_host(_id):
    try:
        h = Host.query.get(_id)
        return jsonify({"m": "cfg", "h": [{"id": h.id, "name": h.name, "ip": h.ip, "mac": h.mac, "type": h.type}]})
    except AttributeError:
        abort(404)

@app.route("/hosts", methods=['GET'])
def get_all_hosts():
    return jsonify({"m": "cfg", "h": map(lambda h: ({"id": h.id, "name": h.name, "ip": h.ip, "mac": h.mac, "type": h.type}), Host.query.all())})
###############################################################################
############### Model objects #################################################

class Node(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    host_id = db.Column(db.ForeignKey("host.id"), nullable=False)
    pin = db.Column(db.Integer, nullable=False)
    name = db.Column(db.String(256))

    def __init__(self, host_id, pin, name):
        self.host_id = host_id
        self.pin = pin
        self.name = name

    def __repr__(self):
        return "<Node host=%s, pin=%s>" % self.host_id, self.pin

class Sensor(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    host_id = db.Column(db.ForeignKey("host.id"), nullable=False)
    pin = db.Column(db.Integer, unique=True, nullable=False)
    name = db.Column(db.String(256))

    def __init__(self, host_id, pin, name):
        self.host_id = host_id
        self.pin = pin
        self.name = name

    def __repr__(self):
        return "<Sensor host=%s, pin=%s>" % self.host_id, self.pin

class Host(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(64), nullable=False, unique=True)
    ip = db.Column(db.String(16), unique=True)
    mac = db.Column(db.String(32), unique=True)
    type = db.Column(db.Enum("S", "C", "E"), nullable=False)

    def __init__(self, name, ip, mac, htype):
        self.name = name
        self.ip = ip
        self.mac = mac
        self.type = htype

    def __repr__(self):
        return "<Host name=%s, ip=%s, mac=%s, type=%s>" % (self.name, self.ip, self.mac, self.type)

class Config(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(32), unique=True, nullable=False)
    value = db.Column(db.String(32))

    def __init__(self, name, value):
        self.name = name
        self.value = value

    def __repr__(self):
        return "<Config name=%s, value=%s>" % (self.name, self.value)

class NodeLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    type = db.Column(db.Enum("nsc", "csr"), nullable=False)
    timestamp = db.Column(db.DateTime(), nullable=False)
    src = db.Column(db.ForeignKey("host.id"), nullable=False, name="src_id")
    dst = db.Column(db.ForeignKey("host.id"), name="dst_id")
    node = db.Column(db.ForeignKey("node.id"), nullable=False, name="node_id")
    state = db.Column(db.Enum("on", "off", "auto"), nullable=False)
    force = db.Column(db.Boolean(), nullable=False)
    force_ts = db.Column(db.DateTime())

    def __init__(self, log_type, timestamp, src, dst, node, state, force, force_ts):
        self.type = log_type
        self.timestamp = timestamp
        self.src = src
        self.dst = dst
        self.node = node
        self.state = state
        self.force = force
        self.force_ts = force_ts

    def __repr__(self):
        return "<NodeLog type=%s, timestamp=%s, node=%s, state=%s, ff=%s, fts=%s>" % (self.type, self.timestamp, self.node.pin, self.state, self.force, self.force_ts)

class SensorLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime(), nullable=False)
    src = db.Column(db.ForeignKey("host.id"), nullable=False, name="src_id")
    dst = db.Column(db.ForeignKey("host.id"), name="dst_id")
    sensor = db.Column(db.ForeignKey("sensor.id"), nullable=False, name="sensor_id")
    value = db.Column(db.Integer(), nullable=False)

    def __init__(self, timestamp, src, dst, sensor, value):
        self.timestamp = timestamp
        self.src = src
        self.dst = dst
        self.sensor = sensor
        self.value = value

    def __repr__(self):
        return "<SensorLog timestamp=%s, sensor=%s, value=%s>" % (self.timestamp, self.sensor.pin, self.value)

class ClockLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime(), nullable=False)
    src = db.Column(db.ForeignKey("host.id"), nullable=False, name="src_id")
    dst = db.Column(db.ForeignKey("host.id"), name="dst_id")
    value = db.Column(db.Integer(), nullable=False)
    overflows = db.Column(db.Integer(), nullable=False)

    def __init__(self, timestamp, src, dst, value, overflows):
        self.timestamp = timestamp
        self.src = src
        self.dst = dst
        self.value = value
        self.overflows = overflows

    def __repr__(self):
        return "<ClockLog timestamp=%s, value=%s, overflows=%s>" % (self.timestamp, self.value, self.overflows)

###############################################################################

db.create_all()

if __name__ == "__main__":
    app.run(host='0.0.0.0')
