/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Util.h
 * File related helper functions.
 * Copyright (C) 2013 Simon Newton
 */

#ifndef INCLUDE_OLA_FILE_UTIL_H_
#define INCLUDE_OLA_FILE_UTIL_H_

#include <string>
#include <vector>

namespace ola {
namespace file {

extern const char PATH_SEPARATOR;

void FindMatchingFiles(const std::string &directory,
                       const std::string &prefix,
                       std::vector<std::string> *files);

void FindMatchingFiles(const std::string &directory,
                       const std::vector<std::string> &prefixes,
                       std::vector<std::string> *files);

/**
 * Convert a path to a filename
 * @param path a full path to a file
 * @param default_value what to return if the path can't be found
 * @return the filename (basename) part of the path or default if it can't be
 *   found
 */
std::string FilenameFromPathOrDefault(const std::string &path,
                                      const std::string &default_value);

/**
 * Convert a path to a filename (this variant is good for switching based on
 *   executable names)
 * @param path a full path to a file
 * @return the filename (basename) part of the path or the whole path if it
 *   can't be found
 */
std::string FilenameFromPathOrPath(const std::string &path);

/**
 * Convert a path to a filename
 * @param path a full path to a file
 * @return the filename (basename) part of the path or "" if it can't be found
 */
std::string FilenameFromPath(const std::string &path);
}  // namespace file
}  // namespace ola
#endif  // INCLUDE_OLA_FILE_UTIL_H_
