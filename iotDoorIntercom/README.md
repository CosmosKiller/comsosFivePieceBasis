# Cosmos SKU5 — dual-image split Matter camera (P4 media + C6 Matter).
#
# Product layout (A+C):
#   shared/include/  — GPIO + EVT headers (P4)
#   p4/              — CosmOS I/O tasks → BRIDGE_EVT_SECURITY_IO (EXTRA into streaming_only)
#   c6/              — security_endpoints.* (EXTRA into esp-matter camera/split_mode)
#
# Upstream media/WebRTC stay as path deps:
#   C6: $ESP_MATTER_PATH/examples/camera/split_mode
#   P4: $KVS_SDK_PATH/examples/streaming_only
#
# Build helpers:
#   ./scripts/build_c6.sh
#   ./scripts/build_p4.sh
#
# See docs/BUILD.md § SKU 5. Matter / OTA / factory-reset policy live on C6;
# P4 senses/actuates via Hosted bridge.
