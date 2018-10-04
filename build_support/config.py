import os.path

from build_support.default_config import *

if os.path.exists("local_config.py"):
  from local_config import *
