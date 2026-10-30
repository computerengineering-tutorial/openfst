import os
import sys
import shutil
import subprocess
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

CURRENT_DIR = Path(__file__).resolve().parent


def find_openfst_root() -> Path:
    # 1. Environment variable if set by CI or user
    env_root = os.environ.get("OPENFST_ROOT_DIR")
    if (
        env_root
        and (Path(env_root) / "CMakeLists.txt").exists()
        and (Path(env_root) / "cmake" / "OpenFstHelpers.cmake").exists()
    ):
        return Path(env_root).resolve()

    # 2. Traverse upwards from this file
    cur = CURRENT_DIR
    for parent in list(cur.parents):
        if (
            (parent / "CMakeLists.txt").exists()
            and (parent / "cmake" / "OpenFstHelpers.cmake").exists()
        ):
            return parent

    # 3. Current working directory
    cwd = Path.cwd().resolve()
    if (
        (cwd / "CMakeLists.txt").exists()
        and (cwd / "cmake" / "OpenFstHelpers.cmake").exists()
    ):
        return cwd

    raise RuntimeError(
        "Could not find OpenFst repository root containing cmake/OpenFstHelpers.cmake"
    )


class CMakeExtension(Extension):
    def __init__(self, name: str):
        super().__init__(name, sources=[])


class CMakeBuild(build_ext):
    def build_extension(self, ext: CMakeExtension) -> None:
        cmake_bin = shutil.which("cmake")
        if not cmake_bin:
            raise RuntimeError("CMake must be installed to build openfst-py")

        build_temp = Path(self.build_temp).resolve()
        build_temp.mkdir(parents=True, exist_ok=True)

        source_dir = find_openfst_root()
        print(f"OpenFst repository root identified at: {source_dir}")

        cfg = "Release"
        cmake_args = [
            f"-DCMAKE_BUILD_TYPE={cfg}",
            "-DOPENFST_ENABLE_PYTHON=ON",
            "-DOPENFST_BUILD_TESTS=OFF",
            "-DOPENFST_ENABLE_BIN=OFF",
            "-DOPENFST_ENABLE_FAR=ON",
            "-DBUILD_SHARED_LIBS=OFF",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            f"-DPython3_EXECUTABLE={sys.executable}",
        ]

        # Use Ninja if available
        if shutil.which("ninja"):
            cmake_args.append("-GNinja")

        # macOS multi-arch / cross-compilation support
        if sys.platform == "darwin":
            archflags = os.environ.get("ARCHFLAGS", "")
            if "-arch arm64" in archflags and "-arch x86_64" in archflags:
                cmake_args.append("-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
            elif "-arch arm64" in archflags:
                cmake_args.append("-DCMAKE_OSX_ARCHITECTURES=arm64")
            elif "-arch x86_64" in archflags:
                cmake_args.append("-DCMAKE_OSX_ARCHITECTURES=x86_64")

            deployment_target = os.environ.get("MACOSX_DEPLOYMENT_TARGET")
            if deployment_target:
                cmake_args.append(f"-DCMAKE_OSX_DEPLOYMENT_TARGET={deployment_target}")

        # Windows MSVC configuration
        if sys.platform == "win32":
            cmake_args.append("-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL")

        # Configure CMake
        print(f"Configuring OpenFst with CMake: {cmake_args}")
        subprocess.check_call(
            [cmake_bin, "-S", str(source_dir), "-B", str(build_temp)] + cmake_args
        )

        # Build target pywrapfst
        jobs = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL") or str(
            os.cpu_count() or 4
        )
        build_args = ["--build", str(build_temp), "--target", "pywrapfst", "--config", cfg, "-j", jobs]
        print(f"Building pywrapfst target: {build_args}")
        subprocess.check_call([cmake_bin] + build_args)

        # Locate compiled pywrapfst binary
        built_files = []
        for pattern in ("pywrapfst*.so", "pywrapfst*.pyd", "pywrapfst*.dylib", "pywrapfst*.dll"):
            built_files.extend(list(build_temp.rglob(pattern)))

        candidate_files = [
            f for f in built_files
            if "CMakeFiles" not in f.parts and f.suffix in {".so", ".pyd", ".dylib", ".dll"}
        ]

        if not candidate_files:
            raise RuntimeError(
                f"Could not find compiled pywrapfst binary in {build_temp}. Files found: {built_files}"
            )

        built_ext_path = candidate_files[0]
        print(f"Found built extension at: {built_ext_path}")

        # Setup destination package directories in build_lib
        build_lib = Path(self.build_lib).resolve()
        build_lib.mkdir(parents=True, exist_ok=True)

        # Place the compiled pywrapfst binary at top-level
        generic_suffix = ".pyd" if sys.platform == "win32" else ".so"
        target_path = Path(self.get_ext_fullpath(ext.name)).resolve()
        target_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(built_ext_path, target_path)

        generic_path = build_lib / f"pywrapfst{generic_suffix}"
        if not generic_path.exists():
            shutil.copy2(built_ext_path, generic_path)

        # Copy top-level type hints pywrapfst.pyi and py.typed
        pyi_source = CURRENT_DIR / "pywrapfst.pyi"
        py_typed_source = CURRENT_DIR / "py.typed"
        if pyi_source.exists():
            shutil.copy2(pyi_source, build_lib / "pywrapfst.pyi")
        if py_typed_source.exists():
            shutil.copy2(py_typed_source, build_lib / "py.typed")

        # Dynamically generate lightweight compatibility packages (openfst & openfst_py)
        # into build_lib so the source tree remains clean without duplicate folders.
        for alias in ("openfst", "openfst_py"):
            alias_dir = build_lib / alias
            alias_dir.mkdir(parents=True, exist_ok=True)
            (alias_dir / "__init__.py").write_text(
                '"""OpenFst compatibility namespace."""\n'
                'from pywrapfst import *  # noqa: F401, F403\n'
                'import pywrapfst  # noqa: F401\n',
                encoding="utf-8",
            )
            (alias_dir / "py.typed").write_text("", encoding="utf-8")
            if pyi_source.exists():
                shutil.copy2(pyi_source, alias_dir / "pywrapfst.pyi")

        # If inplace editable install: pip install -e .
        if self.inplace:
            shutil.copy2(built_ext_path, CURRENT_DIR / target_path.name)
            if not (CURRENT_DIR / f"pywrapfst{generic_suffix}").exists():
                shutil.copy2(built_ext_path, CURRENT_DIR / f"pywrapfst{generic_suffix}")


readme_file = CURRENT_DIR / "README.md"
long_description = readme_file.read_text(encoding="utf-8") if readme_file.exists() else "OpenFst Python bindings"

setup(
    name="openfst-py",
    version="0.1.0",
    author="Jenil Sheth",
    author_email="computerengineeringtutorial@gmail.com",
    description="Stand-alone, multiplatform OpenFst Python bindings with full pywrapfst compatibility",
    long_description=long_description,
    long_description_content_type="text/markdown",
    license="Apache-2.0",
    url="https://github.com/google-research/openfst",
    packages=[],
    py_modules=[],
    package_data={
        "": ["*.pyi", "py.typed"],
    },
    include_package_data=True,
    ext_modules=[CMakeExtension("pywrapfst")],
    cmdclass={"build_ext": CMakeBuild},
    python_requires=">=3.9",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Intended Audience :: Education",
        "Intended Audience :: Science/Research",
        "Operating System :: POSIX :: Linux",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Programming Language :: C++",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Text Processing :: Linguistic",
    ],
    zip_safe=False,
)
