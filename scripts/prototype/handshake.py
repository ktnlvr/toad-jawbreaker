from cryptography.hazmat.primitives.asymmetric.x25519 import (
    X25519PrivateKey,
    X25519PublicKey,
)
from cryptography.hazmat.primitives.kdf.hkdf import HKDF
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.ciphers.aead import ChaCha20Poly1305


import os, base64, time


def b64(b: bytes) -> str:
    return base64.b64encode(b).decode("ascii")


def hb64(s: str) -> bytes:
    return base64.b64decode(s.encode("ascii"))


def generate_key_pair():
    priv = X25519PrivateKey.generate()
    pub = priv.public_key()
    return (priv, pub)


def compute_x25519(priv: X25519PrivateKey, peer_pub_bytes: bytes) -> bytes:
    peer_pub = X25519PublicKey.from_public_bytes(peer_pub_bytes)
    return priv.exchange(peer_pub)


def hkdf_derive(input_key_material: bytes, info: bytes = b"jawbreaker-ftw") -> bytes:
    hk = HKDF(algorithm=hashes.SHA256(), length=64, salt=None, info=info)
    return hk.derive(input_key_material)


def create_initiation(
    initiator_name: str,
    initiator_static_priv: X25519PrivateKey,
    responder_static_pub_bytes: bytes,
):
    initiator_ephemeral_priv, initiator_ephemeral_pub = generate_key_pair()
    initiator_ephemeral_pub_bytes = initiator_ephemeral_pub.public_bytes_raw()

    initiator_static_pub_bytes = initiator_static_priv.public_key().public_bytes(
        encoding=serialization.Encoding.Raw, format=serialization.PublicFormat.Raw
    )

    # C = X25519(initiator_ephemeral_priv, responder_static_pub)
    dh_c = compute_x25519(initiator_ephemeral_priv, responder_static_pub_bytes)
    aead_key = hkdf_derive(dh_c, info=b"init-aead")[:32]
    aead = ChaCha20Poly1305(aead_key)

    timestamp = int(time.time()).to_bytes(8, "big")
    payload = b"initiator:" + initiator_name.encode() + b"|ts:" + timestamp
    nonce = os.urandom(12)
    ct = aead.encrypt(nonce, payload, None)

    msg = {
        "initiator_ephemeral_pub": b64(initiator_ephemeral_pub_bytes),
        "initiator_static_pub": b64(initiator_static_pub_bytes),
        "nonce": b64(nonce),
        "enc_payload": b64(ct),
    }

    return msg, initiator_ephemeral_priv


def respond_to_initiation(
    responder_name: str, responder_static_priv: X25519PrivateKey, init_msg: dict
) -> tuple[dict, bytes, bytes, X25519PrivateKey]:
    """
    Responder processes initiation and returns (response_message, send_key, recv_key, responder_ephemeral_priv).
    send_key is used by responder to encrypt transport to initiator; recv_key is used to decrypt from initiator.
    """
    initiator_ephemeral_pub_bytes = hb64(init_msg["initiator_ephemeral_pub"])
    initiator_static_pub_bytes = hb64(init_msg["initiator_static_pub"])

    responder_ephemeral_priv, responder_ephemeral_pub_bytes = generate_key_pair()

    # C = X25519(responder_static_priv, initiator_ephemeral_pub) -- used to decrypt initiation payload
    dh_c = compute_x25519(responder_static_priv, initiator_ephemeral_pub_bytes)
    aead_init_key = hkdf_derive(dh_c, info=b"init-aead")[:32]
    aead_init = ChaCha20Poly1305(aead_init_key)

    nonce = hb64(init_msg["nonce"])
    ct = hb64(init_msg["enc_payload"])

    # will explode if verification fails
    _ = aead_init.decrypt(nonce, ct, None)

    # compute full DH set (A, B, C) similar to Noise IK variations
    dh_a = compute_x25519(responder_ephemeral_priv, initiator_ephemeral_pub_bytes)
    dh_b = compute_x25519(responder_ephemeral_priv, initiator_static_pub_bytes)
    # dh_c computed above

    ikm = dh_a + dh_b + dh_c
    hk = hkdf_derive(ikm, info=b"handshake")

    # responder send_key = hk[:32], recv_key = hk[32:]
    responder_send_key = hk[:32]
    responder_recv_key = hk[32:]

    # prepare response payload encrypted with responder_send_key
    aead_resp = ChaCha20Poly1305(responder_send_key)
    resp_nonce = os.urandom(12)
    resp_payload = (
        b"responder:"
        + responder_name.encode()
        + b"|ts:"
        + int(time.time()).to_bytes(8, "big")
    )
    resp_ct = aead_resp.encrypt(resp_nonce, resp_payload, None)

    resp_msg = {
        "responder_ephemeral_pub": b64(responder_ephemeral_pub_bytes),
        "nonce": b64(resp_nonce),
        "enc_payload": b64(resp_ct),
    }

    return resp_msg, responder_send_key, responder_recv_key, responder_ephemeral_priv


def finish_handshake(
    initiator_static_priv: X25519PrivateKey,
    initiator_ephemeral_priv: X25519PrivateKey,
    responder_static_pub_bytes: bytes,
    resp_msg: dict,
):
    """
    Initiator finishes handshake given its ephemeral_priv and responder static pub bytes.
    Returns (responder_plaintext, initiator_send_key, initiator_recv_key)
    """
    responder_ephemeral_pub_bytes = hb64(resp_msg["responder_ephemeral_pub"])

    dh_a = compute_x25519(initiator_ephemeral_priv, responder_ephemeral_pub_bytes)
    dh_b = compute_x25519(initiator_static_priv, responder_ephemeral_pub_bytes)
    dh_c = compute_x25519(initiator_ephemeral_priv, responder_static_pub_bytes)

    ikm = dh_a + dh_b + dh_c
    hk = hkdf_derive(ikm, info=b"handshake")

    initiator_send_key = hk[32:]
    initiator_recv_key = hk[:32]

    aead_resp = ChaCha20Poly1305(initiator_recv_key)
    nonce = hb64(resp_msg["nonce"])
    ct = hb64(resp_msg["enc_payload"])
    pt = aead_resp.decrypt(nonce, ct, None)

    return pt, initiator_send_key, initiator_recv_key


def encrypt_transport(send_key: bytes, plaintext: bytes) -> dict:
    aead = ChaCha20Poly1305(send_key)
    nonce = os.urandom(12)
    ct = aead.encrypt(nonce, plaintext, None)
    return {"nonce": b64(nonce), "ct": b64(ct)}


def decrypt_transport(recv_key: bytes, msg: dict) -> bytes:
    aead = ChaCha20Poly1305(recv_key)
    nonce = hb64(msg["nonce"])
    ct = hb64(msg["ct"])
    return aead.decrypt(nonce, ct, None)
