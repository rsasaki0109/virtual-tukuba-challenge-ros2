# third_party

Optional external dependencies live here when needed for local development.

## CageClient

To enable `transport_backend:=cageclient` in `vtc_bridge`, place a checkout at:

```text
/media/sasaki/aiueo/ai_coding_ws/virtual_tukuba_challenge_v2_ws/third_party/CageClient
```

Example:

```bash
git clone --recursive https://github.com/furo-org/CageClient.git third_party/CageClient
```

The build also requires ZeroMQ development headers:

```bash
sudo apt install libzmq3-dev
```
