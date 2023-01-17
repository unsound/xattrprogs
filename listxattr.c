/*-
 * listxattr.c - List the extended attributes of a filesystem node.
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
	const char *path = NULL;
	ssize_t attrlist_size = 0;
	char *attrlist = NULL;

	if(argc != 2) {
		fprintf(stderr, "usage: listxattr <filename>\n");
		goto out;
	}

	path = argv[1];

#if defined(__APPLE__) || defined(__DARWIN__)
	attrlist_size = listxattr(
		path,
		NULL,
		0,
		0);
#elif defined(__linux__)
	attrlist_size = llistxattr(
		path,
		NULL,
		0);
#elif defined(__FreeBSD__)
	attrlist_size = extattr_list_link(
		path,
		EXTATTR_NAMESPACE_USER,
		NULL,
		0);
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	if(attrlist_size == 0) {
#ifdef DEBUG
		fprintf(stderr, "INFO: No extended attributes found "
			"for path \"%s\".\n", path);
#endif
	}
	else if(attrlist_size == -1) {
		fprintf(stderr, "Error while getting size of extended "
			"attribute list for path \"%s\": %s "
			"(errno=%d)\n",
			path, strerror(errno), errno);
		goto out;
	}
	else {
		ssize_t bytes_read = 0;
		ssize_t ptr = 0;

		attrlist = calloc(1, sizeof(char) * (size_t) attrlist_size);
		if(attrlist == NULL) {
			fprintf(stderr, "Error while allocating %zd bytes for "
				"extended attribute list: %s (errno=%d)\n",
				attrlist_size, strerror(errno), errno);
			goto out;
		}

#if defined(__APPLE__) || defined(__DARWIN__)
		bytes_read = listxattr(
			path,
			attrlist,
			attrlist_size,
			0);
#elif defined(__linux__)
		bytes_read = llistxattr(
			path,
			attrlist,
			attrlist_size);
#elif defined(__FreeBSD__)
		bytes_read = extattr_list_link(
			path,
			EXTATTR_NAMESPACE_USER,
			attrlist,
			attrlist_size);
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
		if(bytes_read < 0) {
			fprintf(stderr, "Error while reading extended "
				"attribute list for path \"%s\": %s "
				"(errno=%d)\n",
				path, strerror(errno), errno);
			goto out;
		}
		else if(bytes_read != attrlist_size) {
			fprintf(stderr, "Partial read while reading extended "
				"attribute list for path \"%s\": %zd/%zd bytes "
				"read\n",
				path, bytes_read, attrlist_size);
			goto out;
		}

		while(ptr < attrlist_size) {
#ifdef __FreeBSD__
			char *cur = &attrlist[ptr + 1];
			unsigned char cur_len =
				*((unsigned char*) &attrlist[ptr]);
#else
			char *cur = &attrlist[ptr];
			int cur_len = strlen(cur);
#endif
			fprintf(stdout, "%.*s\n", cur_len, cur);

			ptr += cur_len + 1;
		}

		if(ptr != attrlist_size) {
			fprintf(stderr, "WARNING: ptr (%zd) != attrlist_size "
				"(%zd)\n",
				ptr, attrlist_size);
		}
	}

	ret = (EXIT_SUCCESS);
out:
	if(attrlist) {
		free(attrlist);
	}

	return ret;
}
