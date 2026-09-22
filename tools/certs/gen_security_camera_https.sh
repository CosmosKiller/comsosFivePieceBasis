#!/usr/bin/env bash
# Generate (or regenerate) the Beta self-signed HTTPS cert for iotSecurityCamera /stream.
#
# Usage:
#   ./tools/certs/gen_security_camera_https.sh
#   ./tools/certs/gen_security_camera_https.sh 192.168.1.50   # optional IP SAN for browser tests
#
# After regenerating, rebuild the app so the new PEMs are embedded.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT_DIR="$ROOT/iotSecurityCamera/main/certs"
mkdir -p "$OUT_DIR"

IP_SAN="${1:-}"
SAN="DNS:cosmos-security-camera.local,DNS:localhost"
if [[ -n "$IP_SAN" ]]; then
    SAN="${SAN},IP:${IP_SAN}"
fi

openssl req -newkey rsa:2048 -nodes \
    -keyout "$OUT_DIR/prvtkey.pem" \
    -x509 -days 3650 \
    -out "$OUT_DIR/servercert.pem" \
    -subj "/CN=cosmos-security-camera/O=CosmosKiller/OU=Beta/C=US" \
    -addext "keyUsage=critical,digitalSignature,keyEncipherment" \
    -addext "extendedKeyUsage=serverAuth" \
    -addext "subjectAltName=${SAN}"

chmod 600 "$OUT_DIR/prvtkey.pem"
echo "Wrote $OUT_DIR/servercert.pem and prvtkey.pem"
openssl x509 -in "$OUT_DIR/servercert.pem" -noout -subject -dates -ext subjectAltName
echo "Rebuild iotSecurityCamera to embed the new certificate."
