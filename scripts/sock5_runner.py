import asyncio
from aiohttp_socks import open_connection

HOST = "127.0.0.1"
PORT = 8888
PROXY = "socks5://127.0.0.1:1080"


async def handle_client(reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
    message = "Hello, world!\n"
    writer.write(message.encode())
    await writer.drain()
    writer.close()
    await writer.wait_closed()


async def start_server():
    server = await asyncio.start_server(handle_client, HOST, PORT)
    addr = server.sockets[0].getsockname()
    print(f"Serving on {addr}")
    async with server:
        await server.serve_forever()


async def socks5_client():
    print(f"Connecting to {HOST}:{PORT} via SOCKS5 proxy {PROXY}")
    reader, writer = await open_connection(proxy=PROXY, host=HOST, port=PORT)

    data = await reader.read(100)
    print("Received:", data.decode())

    writer.close()
    await writer.wait_closed()


async def main():
    # Start server in background
    server_task = asyncio.create_task(start_server())
    # Wait briefly for server to start
    await asyncio.sleep(0.1)

    # Run client
    await socks5_client()

    # Shutdown server
    server_task.cancel()
    try:
        await server_task
    except asyncio.CancelledError:
        print("Server shut down.")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("Interrupted")
