# HTTPS certs for `/stream` (Beta)

Self-signed server certificate embedded into firmware for `esp_https_server`.

| File | Role |
|------|------|
| `servercert.pem` | Server certificate (public) |
| `prvtkey.pem` | Server private key (embedded; **Beta only**) |

- CN / SAN: `cosmos-door-intercom` / `cosmos-door-intercom.local`, `localhost`
- Browsers and HA will warn unless you trust this cert; for Home Assistant MJPEG use `verify_ssl: false` (LAN Beta)
- Regenerate: `tools/certs/gen_door_intercom_https.sh [optional-ip]`
- Production path later: per-unit or provisioned certs (not this shared Beta key)
