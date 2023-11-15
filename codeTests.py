#!/usr/bin/env python3

# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 Sky UK
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Build script for running rialto unittests

import subprocess
import os
import argparse
import multiprocessing
import sys

valgrindErrorCode = 101
baseDir = os.getcwd()

def runcmd(okErrorCode, *args, **kwargs):
    status = subprocess.run(*args, **kwargs)
    if status.returncode == okErrorCode or status.returncode == valgrindErrorCode:
        return True
    else:
        args2 = ' '.join(status.args) if type(status.args) == list else status.args
        print(f'Command: "{args2}" returned with {status.returncode} error code!')
    return False

def getSourceFiles():
    rv = []
    for root, dirs, files in os.walk(baseDir):
        for file in files:
            if file.endswith(".cpp") or file.endswith(".h"):
                f = os.path.join(root, file)
                if "/build/" not in f:
                    rv.append(f)
    return rv

def printOk(str):
    if str == "":
        print("    ok")
    else:
        print("    "+str)
    print()
    print("###############################################")
    print()

######################

def doCheckExtra():
    print("Extra checks...")
    files = getSourceFiles()
    # The code shouldn't include iostream
    for file in files:
        executeCmd = ["grep", "-n", "iostream", file, "/dev/null"]
        if not runcmd(1, executeCmd, cwd=baseDir):
            exit(1)

        if file.endswith("ShmCommon.h") or ("/I" in file and file.endswith(".h")):
            continue
        
        #  variable name constant must begin with a k
        executeCmd = [ "egrep", "-n", "^[^\(<]*(const|constexpr) ([a-zA-Z0-9:<>_]+ )+[\*&]*[^km\*&][a-zA-Z0-9_]+[ ]*[=\{]", file, "/dev/null"]
        if not runcmd(1, executeCmd, cwd=baseDir):
            exit(1)
        executeCmd = [ "egrep", "-n", "^[^\(<]*(const|constexpr) ([a-zA-Z0-9:<>_]+ )+[\*&]*m[a-zA-Z0-9][a-zA-Z0-9_]+[ ]*[=\{]", file, "/dev/null"]
        if not runcmd(1, executeCmd, cwd=baseDir):
            exit(1)
    printOk("")

def doCheckDoxygen():
    print("Running doxygen...")
    opdir="build/gh_pages"
    if not os.path.exists(opdir):
        os.mkdir(opdir)
    executeCmd = ["doxygen"]
    if not runcmd(0, executeCmd, cwd=baseDir):
        exit(1)
    errFile = "./doxygen_errors.txt"
    if os.path.isfile(errFile):
        if os.path.getsize(errFile) > 0:
            print()
            print(errFile)
            executeCmd = ["cat", errFile]
            runcmd(0, executeCmd, cwd=baseDir)
            exit(1)
        os.remove(errFile)
    printOk("doxygen test ok")

def doCheckCppcheck():
    print("Running cppcheck...")
    executeCmd = ["cppcheck", "-q", "-ibuild", "--enable=all", "--std=c++17", "--error-exitcode=1",
                  "--suppress-xml=cppcheck_suppressions.xml", "--template='{file}:{line},{severity},{id},{message}'", "."]
    if not runcmd(0, executeCmd, cwd=baseDir):
            exit(1)
    printOk("")

def doCheckCpplint():
    print("Running cpplint...")
    executeCmd = ["python", "scripts/cpplint/cpplint.py", "--recursive", "--output=junit", "."]
    if not runcmd(0, executeCmd, cwd=baseDir):
        exit(1)
    print()
    printOk("cpplint test ok")

def doCheckClang():
    print("Running clang-format-12...")
    files = getSourceFiles()
    for file in files:
        executeCmd = ["clang-format-12", "-style=file", "--dry-run", "--Werror", file]
        if not runcmd(0, executeCmd, cwd=baseDir):
            print()
            print("consider running the following command...");
            print()
            print("  clang-format-12 -i " + file)
            exit(1)
    printOk("")

def doCheckValgrind():
    print("Running valgrind...")
    executeCmd = ["./build_ut.py", "-c", "-xml", "-f", "-val"]
    if not runcmd(0, executeCmd, cwd=baseDir):
        exit(1)
    printOk("valgrind test ok")

######################

def main():
    if not os.path.exists("build"):
        print("You should try building with ./build_ut.py")
        exit(1)


    # Get arguments
    argParser = argparse.ArgumentParser(description='Run code tests.', formatter_class=argparse.RawTextHelpFormatter)
    argParser.add_argument("-a", "--all", action='store_true', help="run all tests")
    argParser.add_argument("-c", "--clang", action='store_true', help="")
    argParser.add_argument("-d", "--doxygen", action='store_true', help="")
    argParser.add_argument("-p", "--cppcheck", action='store_true', help="")
    argParser.add_argument("-l", "--cpplint", action='store_true', help="")
    argParser.add_argument("-v", "--valgrind", action='store_true', help="")
    argParser.add_argument("-e", "--extra", action='store_true', help="")
    args = vars(argParser.parse_args())

    # Run them all if no agument is passed or "-a" or "--all"
    runAll = len(sys.argv) == 1 or args['all'] == True


    
    if runAll or args['extra'] == True:
        doCheckExtra()
        
    if runAll or args['doxygen'] == True:
        doCheckDoxygen()
        
    if runAll or args['cppcheck'] == True:
        doCheckCppcheck()

    if runAll or args['cpplint'] == True:
        doCheckCpplint()

    if runAll or args['clang'] == True:
        doCheckClang()

    if runAll or args['valgrind'] == True:
        doCheckValgrind()

        
if __name__ == "__main__":
    main()
