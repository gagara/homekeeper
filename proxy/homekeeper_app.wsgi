# use virtualenv
activate_this = '/var/www/homekeeper/.pyenv/bin/activate_this.py'
with open(activate_this) as file_:
    exec(file_.read(), dict(__file__=activate_this))

import sys
sys.path.insert(0, '/var/www/homekeeper/proxy')

from homekeeper_proxy import app as application

