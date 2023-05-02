class VmgsIO:

    def __init__(self, **kwargs):
        pass

    def __enter__(self) -> VmgsIO:
        pass

    def __exit__(self, exc_type, exc_val, exc_tb) -> bool:
        pass

    def read(self) -> bytes:
        pass

    def write(self, buf: bytes) -> None:
        pass
