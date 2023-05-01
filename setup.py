import os, sys
import tempfile

import setuptools
import setuptools.command.build_ext

import pybind11
import pybind11.setup_helpers

pybind11.setup_helpers.ParallelCompile(needs_recompile = pybind11.setup_helpers.naive_recompile).install()

class CMakeWorkspace:

    def __init__(self, project_path: str, temp_path: str):
        self.project_path = project_path
        self.temporary_directory = tempfile.TemporaryDirectory(dir = temp_path)

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.temporary_directory.cleanup()

class CMakeExtension(setuptools.Extension):

    def __init__(self, name, target_name, *args, **kwargs) -> None:
        super().__init__(name, sources = [], *args, **kwargs)
        self.cmake_target_name = target_name

class cmake_build_ext(setuptools.command.build_ext.build_ext):

    def build_extensions(self):
        os.makedirs(self.build_lib, exist_ok = True)
        os.makedirs(self.build_temp, exist_ok = True)

        with CMakeWorkspace(os.path.abspath('.'), self.build_temp) as cmake_workspace:
            cmake_defs = []
            if self.define is not None:
                cmake_defs.extend('-D{}'.format(d) for d, _ in self.define)

            self.cmake_source_dir = os.path.abspath(cmake_workspace.project_path)
            self.cmake_build_dir = os.path.abspath(cmake_workspace.temporary_directory.name)

            self.spawn([
                'cmake',
                '-DCMAKE_BUILD_TYPE={}'.format('Debug' if self.debug else 'Release'),
                '-DCMAKE_PREFIX_PATH={}'.format(pybind11.get_cmake_dir()),
                '-DCMAKE_INSTALL_PREFIX={}'.format(os.path.abspath(self.build_lib)),
                '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                *cmake_defs,
                '-B', self.cmake_build_dir,
                '-S', self.cmake_source_dir
            ])

            super().build_extensions()

            self.spawn([ 'cmake', '--install', self.cmake_build_dir ])

    def build_extension(self, ext: CMakeExtension) -> None:
        assert isinstance(ext, CMakeExtension)
        self.spawn([ 'cmake', '--build', self.cmake_build_dir, '--config', 'Debug' if self.debug else 'Release', '--target', ext.cmake_target_name, '--parallel' ])

authors_emails = {
    'Xiao': ('Aiyu Xiao', 'hypersine.git@outlook.com')
}

if __name__ == '__main__':
    setuptools.setup(
        author = authors_emails['Xiao'][0],
        author_email = authors_emails['Xiao'][1],
        packages = [
            'vmgs'
        ],
        package_dir = {
            'vmgs': 'vmgs'
        },
        package_data = {
            'vmgs': [ 'py.typed', '*.pyi' ]
        },
        ext_modules = [
            CMakeExtension('_vmgs', '_vmgs'),
        ],
        cmdclass = {
            'build_ext': cmake_build_ext
        }
    )
