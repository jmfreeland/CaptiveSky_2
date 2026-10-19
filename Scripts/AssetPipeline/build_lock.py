"""Cross-process serialization for builds that share the local asset export root."""

from contextlib import contextmanager
import ctypes
import os
from pathlib import Path
import time
from ctypes import wintypes


class BuildLockError(RuntimeError):
    """Raised when another build owns the shared asset pipeline or locking fails."""


@contextmanager
def pipeline_build_lock(path: Path, timeout_seconds: float = 5.0):
    """Lock a shared output root until all Blender/packing/verifier work finishes.

    Windows file sharing semantics make the lock process-safe and automatically
    release it if the build process exits or crashes. The lock file itself is
    intentionally retained; ownership is represented by the open handle.
    """
    lock_path = Path(path).resolve()
    try:
        lock_path.parent.mkdir(parents=True, exist_ok=True)
        if os.name != "nt":
            raise BuildLockError("Cross-process asset-pipeline locking is currently supported on Windows only.")

        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        create_file = kernel32.CreateFileW
        create_file.argtypes = (
            wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, wintypes.LPVOID,
            wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE,
        )
        create_file.restype = wintypes.HANDLE
        close_handle = kernel32.CloseHandle
        close_handle.argtypes = (wintypes.HANDLE,)
        close_handle.restype = wintypes.BOOL

        invalid_handle = ctypes.c_void_p(-1).value
        deadline = time.monotonic() + max(0.0, timeout_seconds)
        while True:
            handle = create_file(
                str(lock_path), 0x80000000 | 0x40000000, 0,
                None, 4, 0x80, None,
            )
            if handle not in (None, invalid_handle):
                break
            error_code = ctypes.get_last_error()
            if error_code not in (32, 33):
                raise BuildLockError(
                    f"The asset pipeline lock could not be opened (Windows error {error_code})."
                )
            if time.monotonic() >= deadline:
                raise BuildLockError("Another asset build is using this export root; no build was started.")
            time.sleep(0.05)

        try:
            yield
        finally:
            close_handle(handle)
    except BuildLockError:
        raise
    except OSError as exc:
        raise BuildLockError(f"Cannot acquire the asset pipeline lock at {lock_path}: {exc}") from exc
