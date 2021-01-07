import sys
import os
import subprocess

project = 'alia'
copyright = '2021, .decimal, LLC & Mass General Brigham'
author = 'Thomas Madden'

import os
on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

exclude_patterns = ['_build', '_doxygen']

extensions = ['breathe']

if not on_rtd:
    import sphinx_rtd_theme
    extensions.append('sphinx_rtd_theme')
    html_theme = 'sphinx_rtd_theme'

breathe_projects = { "alia": "_doxygen/xml" }
breathe_default_project = "alia"

def generate_doxygen_xml(app):
    subprocess.check_call(['doxygen', 'Doxyfile'])

def setup(app):
    # Add hook for building doxygen xml when needed
    app.connect("builder-inited", generate_doxygen_xml)
