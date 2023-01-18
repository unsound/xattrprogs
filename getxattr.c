/*-
 * getxattr.c - Get the data of one of a filesystem node's extended attributes.
 *
 * Copyright (c) 2023 Erik Larsson
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#if defined(__FreeBSD__)
#include <sys/extattr.h>
#endif
#if defined(__APPLE__) || defined(__DARWIN__) || defined(__linux__)
#include <sys/xattr.h>
#endif

int main(int argc, char **argv)
{
	int ret = (EXIT_FAILURE);
	const char *path = NULL;;
	const char *attr_name = NULL;
	ssize_t attr_size = 0;
	char *attr_data = NULL;
	ssize_t bytes_read;

	if(argc != 3) {
		fprintf(stderr, "usage: getxattr <filename> "
			"<attribute name>\n");
		goto out;
	}

	path = argv[1];
	attr_name = argv[2];

#if defined(__APPLE__) || defined(__DARWIN__)
	attr_size = getxattr(
		path,
		attr_name,
		NULL,
		0,
		0,
		0);
#elif defined(__linux__)
	attr_size = lgetxattr(
		path,
		attr_name,
		NULL,
		0);
#elif defined(__FreeBSD__)
	attr_size = extattr_get_link(
		path,
		EXTATTR_NAMESPACE_USER,
		attr_name,
		NULL,
		0);
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	if(attr_size == -1) {
		fprintf(stderr, "Error while getting size of extended "
			"attribute for path \"%s\" and attribute name "
			"\"%s\": %s (errno=%d)\n",
			path, attr_name, strerror(errno), errno);
		goto out;
	}

	attr_data = calloc(1, sizeof(char) * (attr_size + 1));
	if(attr_data == NULL) {
		fprintf(stderr, "Error while allocating %zd bytes for data "
			"buffer: %s (errno=%d)\n",
			attr_size, strerror(errno), errno);
		goto out;
	}

#if defined(__APPLE__) || defined(__DARWIN__)
	bytes_read = getxattr(
		path,
		attr_name,
		attr_data,
		attr_size,
		0,
		0);
#elif defined(__linux__)
	bytes_read = lgetxattr(
		path,
		attr_name,
		attr_data,
		attr_size);
#elif defined(__FreeBSD__)
	bytes_read = extattr_get_link(
		path,
		EXTATTR_NAMESPACE_USER,
		attr_name,
		attr_data,
		attr_size);
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	if(bytes_read == -1) {
		fprintf(stderr, "Error while getting extended attribute data "
			"for path \"%s\" and attribute name \"%s\": %s "
			"(errno=%d)\n",
			path, attr_name, strerror(errno), errno);
		goto out;
	}
	else if(bytes_read != attr_size) {
		fprintf(stderr, "Partial read while getting extended attribute "
			"data for path \"%s\" and attribute name \"%s\": "
			"%zd/%zd bytes read\n",
			path, attr_name, bytes_read, attr_size);
		goto out;
	}

	if(fwrite(attr_data, attr_size, 1, stdout) != 1) {
		fprintf(stderr, "Error while writing %zd bytes of extended "
			"attribute data to standard output: %s (errno=%d)\n",
			attr_size, strerror(errno), errno);
		goto out;
	}

	ret = (EXIT_SUCCESS);
out:
	if(attr_data) {
		free(attr_data);
	}

	return ret;
}
