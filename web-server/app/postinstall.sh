#!/bin/sh
cp  /usr/local/packages/monkey/reverseproxy.conf /etc/apache2/conf.d/reverseproxy.conf
systemctl reload httpd
