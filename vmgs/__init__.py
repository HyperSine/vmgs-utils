import typing
import json

from ._vmgs import VmgsIO as VmgsIO

def vmgs_encode(data: typing.Dict[str, typing.Any]) -> bytes:
    return json.dumps(data).encode('utf-16-le') + b'\x00\x00'

def vmgs_decode(data: bytes) -> typing.Dict[str, typing.Any]:
    return json.loads(data.decode('utf-16-le').strip('\x00'))
