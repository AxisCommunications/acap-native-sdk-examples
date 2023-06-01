#!/usr/bin/env python3
"""
Copyright (C) 2021, Axis Communications AB, Lund, Sweden

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

--------------------------------------------------------------------------------

Get declared and sent event lists from an Axis device

Requirements:
    Shell: xmllint
        sudo apt install libxml2-utils
    Python: requests
        pip3 install requests

"""
import argparse
import subprocess as subp
import sys
import time
import xml.etree.ElementTree as ET

import requests
from requests.auth import HTTPDigestAuth, HTTPBasicAuth

# Definitions
ONVIFLIST = "onviflist.xml"
SENTLIST = "sentonviflist.xml"
TIMEOUT = 30
APITYPE = "onvif"

# Wrapped commands
XMLLINT_CMD = "xmllint --format -"

# ONVIF - GetEventPropertiesRequest
ONVIF_XML_HEADERS = {'content-type': 'application/xml'}
ONVIF_GEPR_PAYLOAD = """<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope
 xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"
 xmlns:wsa="http://www.w3.org/2005/08/addressing"
 xmlns:tet="http://www.onvif.org/ver10/events/wsdl">
    <SOAP-ENV:Header>
        <wsa:Action>
http://www.onvif.org/ver10/events/wsdl/EventPortType/GetEventPropertiesRequest
 </wsa:Action>
    </SOAP-ENV:Header>
    <SOAP-ENV:Body>
        <tet:GetEventProperties>
 </tet:GetEventProperties>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>"""
# ONVIF - CreatePullPointSubscriptionRequest
#
# It is possible to update from //. to
# e.g. tns1:Monitoring/ProcessorUsage
ONVIF_CPPSR_PAYLOAD = """<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope
 xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"
 xmlns:wsa="http://www.w3.org/2005/08/addressing"
 xmlns:tet="http://www.onvif.org/ver10/events/wsdl"
 xmlns:tns1="http://www.onvif.org/ver10/topics"
 xmlns:wsnt="http://docs.oasis-open.org/wsn/b-2">
    <SOAP-ENV:Header>
        <wsa:Action>
http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest
</wsa:Action>
    </SOAP-ENV:Header>
    <SOAP-ENV:Body>
        <tet:CreatePullPointSubscription>
        <tet:Filter>
<wsnt:TopicExpression Dialect="http://www.onvif.org/ver10/tev/topicExpression/ConcreteSet">//.</wsnt:TopicExpression>
</tet:Filter>
</tet:CreatePullPointSubscription>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>"""
# ONVIF - PullMessagesRequest
ONVIF_PMR_PAYLOAD = """<?xml version="1.0" encoding="UTF-8"?>
<SOAP-ENV:Envelope
 xmlns:SOAP-ENV="http://www.w3.org/2003/05/soap-envelope"
 xmlns:wsa="http://www.w3.org/2005/08/addressing"
 xmlns:tet="http://www.onvif.org/ver10/events/wsdl">
    <SOAP-ENV:Header>
        <wsa:Action>
http://www.onvif.org/ver10/events/wsdl/PullPointSubscription/PullMessagesRequest
 </wsa:Action>
        <wsa:To>http://192.168.0.90/onvif/services</wsa:To>
        <dom0:SubscriptionId xmlns:dom0="http://www.axis.com/2009/event">{}</dom0:SubscriptionId>
    </SOAP-ENV:Header>
    <SOAP-ENV:Body>
        <tet:PullMessages>
            <tet:Timeout>PT5S</tet:Timeout>
            <tet:MessageLimit>1000</tet:MessageLimit>
        </tet:PullMessages>
    </SOAP-ENV:Body>
</SOAP-ENV:Envelope>"""


def get_event_list(args):
    """ Get list of available events to subscribe to from axevent API

        Store event subscribe list to file
    """
    # Set API specific options
    payload = ONVIF_GEPR_PAYLOAD
    xmlfile = ONVIFLIST
    print("\n### Calling ONVIF Event Properties Request ###")
    print("N.B. This option requires an ONVIF user to be registered on\n" +
          "device and those user credentials passed to this call\n")

    # Set proxy options
    proxies = {'http': args.httpproxy,
               'https': args.httpsproxy}
    if args.httpproxy:
        http = "http"
        print("HTTP proxy: {}".format(str(args.httpproxy)))
    elif args.httpsproxy:
        http = "https"
        print("HTTPS proxy: {}".format(str(args.httpsproxy)))
    else:
        http = "http"
    url = "{}://{}/{}/services".format(http, args.ip, APITYPE)
    print("Connect to device with URL: {}".format(url))

    # Set authentication options
    if args.auth == "basic":
        auth = HTTPBasicAuth(args.user, args.password)
        print("Authentication method: {}".format("basic"))
    else:
        auth = HTTPDigestAuth(args.user, args.password)
        print("Authentication method: {}".format("digest"))

    # Make request to device
    response = requests.post(url, headers=ONVIF_XML_HEADERS, auth=auth,
                             data=payload, timeout=TIMEOUT, proxies=proxies)
    response.raise_for_status()

    # Pretty print with xmllint
    if response.status_code == 200:
        response.encoding = 'UTF-8'
        py_ver = sys.version_info
        if py_ver[1] >= 5:
            # Python versions >= 3.5 - standard
            with open(xmlfile, 'w') as xmlf:
                res = subp.run("echo '{}'".format(response.text) + "|" +
                               XMLLINT_CMD, stdout=xmlf, shell=True, check=True)
            if res.returncode:
                with open(xmlfile, 'w') as xmlf:
                    xmlf.write(response.text)
                raise Exception("XML Pretty print with xmllint to file failed")
        else:
            # Python versions < 3.5
            with open(xmlfile, 'w') as xmlf:
                res = subp.call("echo '{}'".format(response.text) + "|" +
                                XMLLINT_CMD, stdout=xmlf, shell=True)
            if res:
                with open(xmlfile, 'w') as xmlf:
                    xmlf.write(response.text)
                raise Exception("XML Pretty print with xmllint to file failed")


def get_sent_list(args):
    """ Get list of sent events from ONVIF API

        Store sent event list to file
    """
    # Set API specific options
    payload = ONVIF_CPPSR_PAYLOAD
    xmlfile = SENTLIST
    print("\n### Calling ONVIF Create Pull Point Subscription Request and\n" +
          "ONVIF Pull Messages Request###")
    print("N.B. This option requires an ONVIF user to be registered on\n" +
          "device and those user credentials passed to this call\n")

    # Set proxy options
    proxies = {'http': args.httpproxy,
               'https': args.httpsproxy}
    if args.httpproxy:
        http = "http"
        print("HTTP proxy: {}".format(str(args.httpproxy)))
    elif args.httpsproxy:
        http = "https"
        print("HTTPS proxy: {}".format(str(args.httpsproxy)))
    else:
        http = "http"
    url = "{}://{}/{}/services".format(http, args.ip, APITYPE)
    print("Connect to device with URL: {}".format(url))

    # Set authentication options
    if args.auth == "basic":
        auth = HTTPBasicAuth(args.user, args.password)
        print("Authentication method: {}".format("basic"))
    else:
        auth = HTTPDigestAuth(args.user, args.password)
        print("Authentication method: {}".format("digest"))

    # Make CreatePullPointSubscription request to device
    response = requests.post(url, headers=ONVIF_XML_HEADERS, auth=auth,
                             data=payload, timeout=TIMEOUT, proxies=proxies)
    response.raise_for_status()

    if response.status_code == 200:
        root = ET.fromstring(response.content)
        for child in root.iter('*'):
            if "SubscriptionId" in str(child.tag):
                subscription_id = child.text

    if subscription_id:
        print("Subscription id: {}".format(subscription_id))
    else:
        raise Exception("SubscriptionId is not found in response")

    # Wait before PullMessages request to get more events
    print("Waiting for {} seconds...".format(args.time))
    time.sleep(int(args.time))
    print("Waiting finished")

    # Make PullMessages request to device
    payload = ONVIF_PMR_PAYLOAD.format(subscription_id)
    response = requests.post(url, headers=ONVIF_XML_HEADERS, auth=auth,
                             data=payload, timeout=TIMEOUT, proxies=proxies)
    response.raise_for_status()

    # Pretty print with xmllint
    if response.status_code == 200:
        response.encoding = 'UTF-8'
        py_ver = sys.version_info
        if py_ver[1] >= 5:
            # Python versions >= 3.5 - standard
            with open(xmlfile, 'w') as xmlf:
                res = subp.run("echo '{}'".format(response.text) + "|" +
                               XMLLINT_CMD, stdout=xmlf, shell=True, check=True)
            if res.returncode:
                with open(xmlfile, 'w') as xmlf:
                    xmlf.write(response.text)
                raise Exception("XML Pretty print with xmllint to file failed")
        else:
            # Python versions < 3.5
            with open(xmlfile, 'w') as xmlf:
                res = subp.call("echo '{}'".format(response.text) + "|" +
                                XMLLINT_CMD, stdout=xmlf, shell=True)
            if res:
                with open(xmlfile, 'w') as xmlf:
                    xmlf.write(response.text)
                raise Exception("XML Pretty print with xmllint to file failed")


def main():
    """ Parse arguments and call correct method """

    # Parse arguments - top level
    description = "Get list of events from an Axis device.\n"
    parser = argparse.ArgumentParser(description=description)
    subparsers = parser.add_subparsers()

    # Get list arguments
    get_list = subparsers.add_parser("getlist", help='Get list of declared ' +
                                     'events from ONVIF API')

    get_list.add_argument("-a", "--auth", default="digest",
                          help="Authentication method: basic, digest (def: digest)")
    get_list.add_argument("-u", "--user", default="root",
                          help="ONVIF User name on device (def: root)")
    get_list.add_argument("-p", "--password", default="pass",
                          help="Password for ONVIF user on device (def: pass)")
    get_list.add_argument("-i", "--ip", default="192.168.0.90",
                          help="IP-address to Axis device (def: 192.168.0.90)")
    get_list.add_argument("--httpproxy", default=None,
                          help="Optional http proxy (def: None)")
    get_list.add_argument("--httpsproxy", default=None,
                          help="Optional https proxy (def: None)")
    get_list.set_defaults(func=get_event_list)

    # Get sent arguments
    get_list = subparsers.add_parser("getsent", help='Get list of sent ' +
                                     'events from ONVIF API')

    get_list.add_argument("-a", "--auth", default="digest",
                          help="Authentication method: basic, digest (def: digest)")
    get_list.add_argument("-u", "--user", default="root",
                          help="ONVIF User name on device (def: root)")
    get_list.add_argument("-p", "--password", default="pass",
                          help="Password for ONVIF user on device (def: pass)")
    get_list.add_argument("-i", "--ip", default="192.168.0.90",
                          help="IP-address to Axis device (def: 192.168.0.90)")
    get_list.add_argument("-t", "--time", default="10",
                          help="Time, in seconds, for fetching events  (def: 10)")
    get_list.add_argument("--httpproxy", default=None,
                          help="Optional http proxy (def: None)")
    get_list.add_argument("--httpsproxy", default=None,
                          help="Optional https proxy (def: None)")
    get_list.set_defaults(func=get_sent_list)

    # Parse arguments
    args = parser.parse_args()
    if args != argparse.Namespace():
        args.func(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
