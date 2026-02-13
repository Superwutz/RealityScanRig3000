# Counter Worker (Cloudflare)

This worker provides a simple global install counter API for the web installer.

## Endpoints

- `GET /counter?key=installer-success-total` -> `{ "ok": true, "key": "...", "value": 123 }`
- `GET /counter/hit?key=installer-success-total` -> increments and returns the new value

## Setup

1. Install Wrangler:
```bash
npm i -g wrangler
```

2. Create a KV namespace:
```bash
wrangler kv namespace create SCANRIG_COUNTER_KV
```

3. Copy the returned `id` into `wrangler.toml` (`kv_namespaces` binding `SCANRIG_COUNTER_KV`).

4. Deploy:
```bash
cd deploy/counter-worker
wrangler deploy
```

5. In `deploy/web-installer/index.html` add your worker base URL once:
```html
<script>
  window.SCANRIG_COUNTER_WORKER_BASE = "https://your-worker.your-subdomain.workers.dev";
</script>
```

Without this variable, installer falls back to CountAPI.
