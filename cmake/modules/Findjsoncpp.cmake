# - Try to find jsoncpp
# Once done this will define
#  jsoncpp_FOUND - system has jsoncpp
#  jsoncpp_INCLUDE_DIRS - include directories for jsoncpp
#  jsoncpp_LIBRARIES - libraries for jsoncpp

FIND_PATH(jsoncpp_INCLUDE_DIRS json/json.h
          PATH_SUFFIXES jsoncpp)
FIND_LIBRARY(jsoncpp_LIBRARIES NAMES jsoncpp)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(jsoncpp DEFAULT_MSG
                                  jsoncpp_LIBRARIES jsoncpp_INCLUDE_DIRS)

MARK_AS_ADVANCED(jsoncpp_INCLUDE_DIRS jsoncpp_LIBRARIES)
