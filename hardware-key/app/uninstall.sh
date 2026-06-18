#!/bin/sh

CERTSET_NAME="HelloCertFromScript"

dbus-send --print-reply --system --dest=com.axis.PolicyKitCert --type=method_call /com/axis/PolicyKitCert com.axis.PolicyKitCert.CertSetDeleteUnpriv string:"${CERTSET_NAME}"

exit 0
