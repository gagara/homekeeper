FROM python:3.5-alpine

ADD requirements.txt /homekeeper/

RUN set -ex; \
    pip install -r /homekeeper/requirements.txt

ADD .ssl/* /homekeeper/.ssl/

ADD gateway.py gateway.conf /homekeeper/

WORKDIR /homekeeper

EXPOSE 5000

ENTRYPOINT ["python", "gateway.py"]

