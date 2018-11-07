FROM python:3.5-alpine

ADD gateway.py gateway.conf requirements.txt /homekeeper/
ADD .ssl/* /homekeeper/.ssl/

RUN set -ex; \
    pip install -r /homekeeper/requirements.txt

WORKDIR /homekeeper

EXPOSE 8088

ENTRYPOINT ["python", "gateway.py"]

