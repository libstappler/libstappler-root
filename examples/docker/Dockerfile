FROM sbkarr/libstappler-root:latest
RUN \
	apk add libstdc++ make cmake gcc g++ git pkgconfig perl linux-headers binutils-gold \
	&& cd /opt \
	&& mkdir httpd \
	&& cd httpd \
	&& wget https://dlcdn.apache.org/httpd/httpd-2.4.62.tar.bz2 \
	&& tar -xf httpd-2.4.62.tar.bz2 \
	&& rm httpd-2.4.62.tar.bz2 \
	&& mv httpd-2.4.62 httpd \
	&& wget https://dlcdn.apache.org//apr/apr-1.7.5.tar.bz2 \
	&& tar -xf apr-1.7.5.tar.bz2 \
	&& rm apr-1.7.5.tar.bz2 \
	&& mv apr-1.7.5 httpd/srclib/apr \
	&& wget https://dlcdn.apache.org//apr/apr-util-1.6.3.tar.bz2 \
	&& tar -xf apr-util-1.6.3.tar.bz2 \
	&& rm apr-util-1.6.3.tar.bz2 \
	&& mv apr-util-1.6.3 httpd/srclib/apr-util \
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
	&& rm -rf /opts/httpd \
	&& cd /opt/libstappler-root/examples/docker/web \
	&& make APACHE_HTTPD_INCLUDE=/opt/apache/include RELEASE=1 install \
	&& mkdir /opt/libstappler-root/examples/docker/web/www \
	&& cd /opt/libstappler-root/examples/docker/web/conf \
	&& apk add openssl \
	&& openssl req -x509 -out localhost.crt -keyout localhost.key   -newkey rsa:2048 -nodes \
		-sha256   -subj /CN=localhost -extensions EXT -config <( \
		printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth") \
	&& rm -rf /opt/libstappler-root/examples/docker/web/stappler-build/host/release \
	&& apk del make cmake gcc g++ git pkgconfig perl linux-headers binutils-gold openssl
EXPOSE 8080
EXPOSE 8443
