FROM logstash:2.4-alpine

RUN set -ex; \
	/opt/logstash/bin/logstash-plugin install logstash-filter-elasticsearch

ADD logstash_hk.conf /opt/logstash/pipeline/logstash_hk.conf
ADD template_hk.json /opt/logstash/config/template_hk.json 

CMD ["-f", "/opt/logstash/pipeline/logstash_hk.conf"]

