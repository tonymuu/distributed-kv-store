#!/usr/bin/env python3

#**********************
#*
#* Progam Name: MP2. Fault-Tolerant Key-Value Store
#*
#* Current file: submit.py
#* About this file: Submission python script.
#*
#***********************

# UPDATE LOG:
# Originally designed for Python 2.
# Updated 20191024 with Python2/3 compatibility, further diagnostics
# Updated Oct 2020 by CSIDs for MP revision, h/cpp source upload as zip,
#     and for API v2; partIds injected by prerelease build step

# MAINTAINER NOTES:
# Make sure the assignment key (akey) and Part IDs (partIds) in this script
# are up to date for the current installation of the assignment. Look in the
# definition of the submit function.

# This message was primarily meant for MOOC students
def display_prereqs_notice():
  print('\nREQUIREMENTS: To work on this assignment, we assume you already know how to')
  print('program in C++, you have a working Bash shell environment set up (such as')
  print('Linux, or Windows 10 with WSL installed for Ubuntu Linux and a Bash shell,')
  print('or macOS with developer tools installed and possibly additional Homebrew')
  print('tools. If you are not clear yet what these things are, then you need to')
  print('take another introductory course before working on this assignment. The')
  print('University of Illinois offers some intro courses on Coursera that will help')
  print('you understand these things and set up your work environment.\n')

import hashlib
import random
import email
import email.message
import email.encoders
# import StringIO # unused
import sys
import subprocess
import json
import os
import os.path
import base64
from io import BytesIO
from zipfile import ZipFile

# Python2/3 compatibility hacks: ----------------------------

# The script looks for this file to make sure it's the right working directory
anchoring_file = 'Application.cpp'

# Message displayed if compatibility hacks fail
compat_fail_msg = '\n\nERROR: Python 3 compatibility fix failed.\nPlease try running the script with the "python2" command instead of "python" or "python3".\n\n'
wrong_dir_msg = '\n\nERROR: Please run this script from the same directory where ' + anchoring_file + ' is.\n\n'

if not os.path.isfile(anchoring_file):
  print(wrong_dir_msg)
  raise Exception(wrong_dir_msg)
else:
  #print('Found file: ' + anchoring_file)
  pass

try:
  raw_input
except:
  # NameError
  try:
    raw_input = input
  except:
    raise Exception(compat_fail_msg)

# urllib2 hacks based on suggestions by Ed Schofield.
# Link: https://python-future.org/compatible_idioms.html?highlight=urllib2
try:
  # Python 2 versions
  from urlparse import urlparse
  from urllib import urlencode
  from urllib2 import urlopen, Request, HTTPError
except ImportError:
  # Python 3 versions
  try:
    from urllib.parse import urlparse, urlencode
    from urllib.request import urlopen, Request
    from urllib.error import HTTPError
  except:
    raise Exception(compat_fail_msg)

# End of compatibility hacks ---------------------------

# Begin submission stuff -------------------------------

# ---
# The "### INJECTION_SITE" is a magic comment. Don't change it.
# The results should resemble, e.g.:
# injected_akey = "asdfqwertzxcv"
# injected_partIds = ['TyUiO', 'GhUjI', 'KjIuY', 'EfGhR']
# injected_partNames = ['A Test', 'B Test', 'C Test', 'D Test']
injected_akey = "XYM7SFBlTi6tUpn2VVuVUw"
injected_partIds = ['ftlBH', 'Zt9JS', 'rSCvU', 'jcqxK']
injected_partNames = ['Create Test', 'Delete Test', 'Read Test', 'Update Test']
### INJECTION_SITE
# ---

def get_injected_akey():
  global injected_akey
  return injected_akey.strip()

def get_injected_partIds():
  global injected_partIds
  for i in range(len(injected_partIds)):
    injected_partIds[i] = injected_partIds[i].strip()
    assert "" != injected_partIds[i], "partId can't be blank"
  return injected_partIds

def get_injected_partNames():
  global injected_partNames
  for i in range(len(injected_partNames)):
    injected_partNames[i] = injected_partNames[i].strip()
    assert "" != injected_partNames[i], "part displayName can't be blank"
  return injected_partNames

def assert_injections():
  try:
    akey = get_injected_akey()
    assert "" != akey , "akey is empty string"
    partIds = get_injected_partIds()
    assert len(partIds) != 0 , "partIds has 0 length"
    partNames = get_injected_partNames()
    assert len(partNames) != 0 , "partNames has 0 length"
    assert len(partNames) == len(partIds) , "partIds and partNames have different lengths"
  except Exception as e:
    msg = "Couldn't read submission part ID data. Please contact the course staff about this."
    msg += " Original error: " + str(e)
    raise Exception(msg)
  return True

def submit():
  print('==\n== Submitting Solutions \n==')

  (login_email, token) = loginPrompt()
  if not login_email:
    print('!! Submission Cancelled')
    return

  # Old stuff [Pre-2020]
  # debug log runner:
  # script_process = subprocess.Popen(['bash', 'run.sh', str(0)])
  # output = script_process.communicate()[0]
  # return_code = script_process.returncode
  # if return_code is not 0:
  #   raise Exception('ERROR: Build script failed during compilation. See error messages above.')
  # submissions = [read_dbglog(i) for i in range(3)]

  # Old stuff [Mid-2020]
  # src = read_codefile("MP1Node.cpp")
  # submissions = [src, src, src]
  # submitSolution(login_email, token, submissions)

  # Old stuff [Before API v2 re-release, late 2020]
  # # Assignment Key from the programming assignment setup:
  # akey = 'wrongKeyPleaseUpdate'
  # # The Part ID shown on Coursera when this grading part was created:
  # partIds = ['jFrjo', 'BguRo', 'qQXef', 'JQTiA']
  # # Descriptive names for each part to use in messages in this script:
  # # (These don't have to match anything in the Coursera setup exactly)
  # partNames = ['Create Test', 'Delete Test', 'Read Test', 'Update Test']

  # Current stuff [Oct 2020]
  assert_injections()
  akey = get_injected_akey()
  partIds = get_injected_partIds()
  partNames = get_injected_partNames()

  # Currently submitting the same bundle for each part
  useOwn = useOwnMP1Prompt()
  if useOwn:
    b64zip = b64zip_from_files(["MP1Node.h", "MP1Node.cpp", "MP2Node.h", "MP2Node.cpp"])
  else:
    b64zip = b64zip_from_files(["MP2Node.h", "MP2Node.cpp"])
  submissions = [b64zip for i in partIds]

  submitSolution(login_email, token, akey, submissions, partNames, partIds)

# =========================== LOGIN HELPERS - NO NEED TO CONFIGURE THIS =======================================

class NullDevice:
  def write(self, s):
    pass

def getYesNoAnswer(msg):
  while True:
    yn = raw_input(msg)
    yn += 'z'
    yn = yn.strip().lower()
    if yn[0] == 'y':
      print('  You answered "yes".')
      return True
    elif yn[0] == 'n':
      print('  You answered "no".')
      return False
    else:
      print('  Invalid input. Enter y or n.')

def useOwnMP1Prompt():
  """Ask user whether they want to use their own MP1 solution or the reference solution"""
  print('\nGRADING NOTE: This MP relies on the MP1Node.h and MP1Node.cpp files too.')
  print('You can fill out those files with your own solution from MP1.')
  print('Otherwise, we will inject the reference MP1 solution when')
  print('we grade your MP2 submission on the server.\n')
  useOwn = getYesNoAnswer('Use your own MP1Node.h/cpp for grading? (y/n:) ')
  if useOwn:
    print("  We'll grade with your own MP1Node files from this directory.")
    return True
  else:
    print("  We'll grade with solution copies of MP1Node files.")
    return False

def loginPrompt():
  """Prompt the user for login credentials. Returns a tuple (login, token)."""
  (login_email, token) = basicPrompt()
  return login_email, token

def basicPrompt():
  """Prompt the user for login credentials. Returns a tuple (login, token)."""
  print('Please enter the email address that you use to log in to Coursera.')
  login_email = raw_input('Login (Email address): ')
  print('To validate your submission, we need your submission token.\nThis is the single-use key you can generate on the Coursera instructions page for this assignment.\nThis is NOT your own Coursera account password!')
  token = raw_input('Submission token: ')
  return login_email, token

def partPrompt(partNames, partIds):
  print('Hello! These are the assignment parts that you can submit:')
  counter = 0
  for part in partNames:
    counter += 1
    print(str(counter) + ') ' + partNames[counter - 1])
  partIdx = int(raw_input('Please enter which part you want to submit (1-' + str(counter) + '): ')) - 1
  return (partIdx, partIds[partIdx])

def submit_url():
  """Returns the submission url."""
  return "https://www.coursera.org/api/onDemandProgrammingScriptSubmissions.v1"

def submitSolution(email_address, token, akey, submissions, partNames, partIds):
  """Submits a solution to the server. Returns (result, string)."""

  num_submissions = len(submissions)
  if len(partNames) != num_submissions:
    raise Exception('Config error: need one part name per submission item')
  if len(partIds) != num_submissions:
    raise Exception('Config error: need one part ID per submission item')

  parts_dict = dict()
  i = 0
  for p_ in partIds:
    parts_dict[partIds[i]] = {"output": submissions[i]}
    i += 1

  values = {
    "assignmentKey": akey,
    "submitterEmail": email_address,
    "secret": token,
    "parts": parts_dict
  }
  url = submit_url()
  # (Compatibility update) Need to encode as utf-8 to get bytes for Python3:
  data = json.dumps(values).encode('utf-8')
  req = Request(url)
  req.add_header('Content-Type', 'application/json')
  req.add_header('Cache-Control', 'no-cache')
  response = urlopen(req, data)
  return

## Read a debug log
def read_dbglog(partIdx):
  # open the file, get all lines
  f = open("dbg.%d.log" % partIdx)
  src = f.read()
  f.close()
  #print src
  return src

## Read a source code file
def read_codefile(filename):
  # open the file, get all lines
  f = open(filename)
  src = f.read()
  f.close()
  #print src
  return src

# Given a list of filenames, construct a zipfile in memory,
# and return it encoded as a b64 string
def b64zip_from_files(filenames):
  if len(filenames) < 1:
    raise Exception("filenames list is empty")
  buf = BytesIO()
  zipbuf = ZipFile(buf, mode='w')
  for filename in filenames:
    zipbuf.write(filename)
  zipbuf.close()
  buf.seek(0)
  b64str = base64.b64encode(buf.read()).decode('ascii')
  buf.close()
  return b64str

def cleanup():
    for i in range(3):
        try:
            os.remove("dbg.%s.log" % i)
        except:
            pass

def main():
  try:
    submit()
    print('\n\nSUBMISSION FINISHED!\nYou can check your grade on Coursera.\n\n');
  except HTTPError:
    print('ERROR:\nSubmission authorization failed. Please check that your submission token is valid.')
    print('You can generate a new submission token on the Coursera instructions page\nfor this assignment.')

if __name__ == "__main__":
  args = sys.argv[1:]
  if len(args) > 0 and "--validate-injection" == args[0]:
    assert_injections()
  else:
    display_prereqs_notice()
    main()

# For dbg.*.log only:
# cleanup()
