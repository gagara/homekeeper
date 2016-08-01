from flask import Flask

app = Flask(__name__)

@app.route('/<cmd>', methods=['GET'])
@app.route('/<cmd>/<nid>', methods=['GET'])
@app.route('/<cmd>/<nid>/<nstate>', methods=['GET'])
@app.route('/<cmd>/<nid>/<nstate>/<nts>', methods=['GET'])
def get_request(cmd, nid=None, nstate=None, nts=None):
    return 'cmd: %s; id: %s; state: %s; ts: %s' % (cmd, nid, nstate, nts)

if __name__ == "__main__":
    app.run(host='0.0.0.0')
