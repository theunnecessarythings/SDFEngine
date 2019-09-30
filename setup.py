from distutils.core import setup, Extension
setup(name = 'sdf_engine', version = '1.0',  \
   ext_modules = [Extension('sdf_engine', ['sdf_engine.c'], extra_link_args=["-lGLEW", "-lglfw"])])