const DEFAULT_KEY = "installer-success-total";

function json(body, status = 200) {
  return new Response(JSON.stringify(body), {
    status,
    headers: {
      "content-type": "application/json; charset=utf-8",
      "access-control-allow-origin": "*",
      "access-control-allow-methods": "GET, OPTIONS",
      "access-control-allow-headers": "content-type",
      "cache-control": "no-store",
    },
  });
}

function parseKey(url) {
  const raw = url.searchParams.get("key") || DEFAULT_KEY;
  // Keep keys predictable and safe for KV.
  const key = raw.trim().toLowerCase().replace(/[^a-z0-9._-]/g, "");
  return key || DEFAULT_KEY;
}

async function getValue(env, key) {
  const stored = await env.SCANRIG_COUNTER_KV.get(key);
  const n = Number(stored);
  return Number.isFinite(n) && n >= 0 ? Math.floor(n) : 0;
}

export default {
  async fetch(request, env) {
    if (request.method === "OPTIONS") {
      return json({ ok: true }, 204);
    }
    if (request.method !== "GET") {
      return json({ ok: false, error: "method not allowed" }, 405);
    }

    const url = new URL(request.url);
    const key = parseKey(url);

    if (url.pathname === "/counter") {
      const value = await getValue(env, key);
      return json({ ok: true, key, value });
    }

    if (url.pathname === "/counter/hit") {
      const current = await getValue(env, key);
      const next = current + 1;
      await env.SCANRIG_COUNTER_KV.put(key, String(next));
      return json({ ok: true, key, value: next });
    }

    return json({ ok: false, error: "not found" }, 404);
  },
};
