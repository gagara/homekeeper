from flask import Flask, request

app = Flask(__name__)

@app.route("/", methods=['POST'])
def post_request():
    print('REQUEST: %s: %s' % (request.headers, request.data))
    return ('', 200)

if __name__ == "__main__":
    app.run(host='0.0.0.0')
