#!/usr/bin/env python

from distutils.core import setup

setup(
    name='python-beancashrpc',
    version='1.3',
    description='Enhanced version of python-jsonrpc for use with Bean Cash',
    long_description=open('README.rst').read(),
    author='Shawn Kent',
    author_email='<admin@beancash.org>',
    maintainer='Shawn Kent',
    maintainer_email='<admin@beancash.org>',
    url='http://www.github.com/teambean/python-beancashrpc',
    packages=['beancashrpc'],
    classifiers=[
        'License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)', 'Operating System :: OS Independent'
    ]
)
