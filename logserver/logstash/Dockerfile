FROM docker.elastic.co/logstash/logstash:6.3.1

RUN set -ex; \
	/usr/share/logstash/bin/logstash-plugin install logstash-filter-elasticsearch logstash-filter-aggregate

ADD logstash_hk.conf /usr/share/logstash/pipeline/logstash_hk.conf
ADD template_hk.json /usr/share/logstash/config/template_hk.json 

CMD ["-f", "/usr/share/logstash/pipeline/logstash_hk.conf", "-w", "1"]

