import sys
import os
import subprocess

import sphinx_rtd_theme

project = 'alia'
copyright = '2021, .decimal, LLC & Mass General Brigham'
author = 'Thomas Madden'

extensions = ['breathe', 'sphinx_rtd_theme']

exclude_patterns = ['_build', '_doxygen']

html_theme = 'sphinx_rtd_theme'

breathe_projects = { "alia": "_doxygen/xml" }
breathe_default_project = "alia"

def generate_doxygen_xml(app):
    subprocess.check_call(['doxygen', 'Doxyfile'])

def setup(app):
    # Add hook for building doxygen xml when needed
    app.connect("builder-inited", generate_doxygen_xml)
