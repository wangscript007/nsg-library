TEMPLATE = subdirs
CONFIG += testcase
DIRS = $$files(*)
DIRS -= tests.pro
SUBDIRS = $$DIRS
