#!/bin/sh
rm /etc/apache2/conf.d/reverseproxy.conf
systemctl reload httpd
