from setuptools import setup, Extension

setup(
    name="sensor_processor",
    version="1.0",
    ext_modules=[Extension("sensor_processor", ["sensor_processor.c"])],
)
