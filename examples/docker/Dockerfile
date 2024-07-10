FROM sbkarr/libstappler-root
RUN \
	apk add make cmake gcc g++ git pkgconfig perl linux-headers binutils-gold \
	&& cd /opt \
	&& mkdir httpd \
	&& cd httpd \
	&& wget https://dlcdn.apache.org/httpd/httpd-2.4.61.tar.bz2 \
	&& tar -xf httpd-2.4.61.tar.bz2 \
	&& rm httpd-2.4.61.tar.bz2 \
	&& mv httpd-2.4.61 httpd \
	&& wget https://dlcdn.apache.org//apr/apr-1.7.4.tar.bz2 \
	&& tar -xf apr-1.7.4.tar.bz2 \
	&& rm apr-1.7.4.tar.bz2 \
	&& wget https://dlcdn.apache.org//apr/apr-util-1.6.3.tar.bz2 \
	&& tar -xf apr-util-1.6.3.tar.bz2 \
	&& rm apr-util-1.6.3.tar.bz2 \
	&& mv apr-1.7.4 httpd-2.4.59/srclib/apr \
	&& mv apr-util-1.6.3 httpd-2.4.59/srclib/apr-util \
	&& wget https://github.com/PCRE2Project/pcre2/releases/download/pcre2-10.43/pcre2-10.43.tar.bz2 \
	&& tar -xf pcre2-10.43.tar.bz2 \
	&& rm pcre2-10.43.tar.bz2 \
	&& cd pcre2-10.43 \
	&& ./configure \
	&& make \
	&& make install \
	&& cd .. \
	&& wget https://github.com/libexpat/libexpat/releases/download/R_2_6_2/expat-2.6.2.tar.bz2 \
	&& tar -xf expat-2.6.2.tar.bz2 \
	&& rm expat-2.6.2.tar.bz2 \
	&& cd expat-2.6.2 \
	&& ./configure \
	&& make \
	&& make install \
	&& cd .. \
	&& cd httpd \
	&& export STAPPLER_DEPS_ROOT=/opt/libstappler-root/deps/linux/x86_64 \
	&& chmod +x /opt/libstappler-root/deps/replacements/httpd/configure-stappler.sh \
	&& /opt/libstappler-root/deps/replacements/httpd/configure-stappler.sh /opt/apache \
	&& make \
	&& make install \
	&& cd .. \
	&& rm -rf /opts/httpd

# To build image:
# $ docker build -t libstappler-example-docker .

# To build example within container:
# $ cd /opt/libstappler-root/examples/docker/web
# $ make APACHE_HTTPD_INCLUDE=/opt/apache/include install

# To run example:
# $ /opt/apache/bin/httpd -X -f /opt/libstappler-root/examples/docker/web/stappler-build/httpd.conf

CMD ["/bin/sh"]
