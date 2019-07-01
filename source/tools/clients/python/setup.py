import os
from setuptools import setup

project_root = os.path.dirname(os.path.realpath(__file__))
requirementPath = os.path.join(project_root, 'requirements.txt')
install_requires = []
with open(requirementPath) as f:
    install_requires = f.read().splitlines()

setup(name='zero_ad',
      version='0.0.2',
      description='Python client for 0 AD',
      url='http://github.com/brollb/0ad',
      author='Brian Broll',
      author_email='brian.broll@gmail.com',
      install_requires=install_requires,
      license='MIT',
      packages=['zero_ad'],
      zip_safe=False)
