# HTTPS certs for `/stream` (Beta)

Self-signed PEMs embedded into `iotSecurityCamera` for HTTPS MJPEG.

Regenerate:

```bash
./tools/certs/gen_security_camera_https.sh
# optional IP SAN:
./tools/certs/gen_security_camera_https.sh 192.168.1.50
```

Then rebuild the app.
