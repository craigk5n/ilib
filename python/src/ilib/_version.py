"""Single source of truth for the package version.

Kept free of imports so the build backend can read ``__version__`` statically
(via AST) without importing the package -- importing :mod:`ilib` would try to
``dlopen`` the C library, which need not be present at build time.
"""

__version__ = "0.1.0"
