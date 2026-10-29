"""Shared assertions for graphics regression tests."""


def check(condition, message):
    if not condition:
        raise RuntimeError(message)
