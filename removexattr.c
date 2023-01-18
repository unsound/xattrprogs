/*-
 * removexattr.c - Remove an extended attribute from a filesystem node.
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

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#include <fcntl.h>
#include <unistd.h>
#endif
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
	const char *attr_name = NULL;
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	int attrdirfd = -1;
#endif

	if(argc != 3) {
		fprintf(stderr, "usage: removexattr <filename> "
			"<attribute name>\n");
		goto out;
	}

	path = argv[1];
	attr_name = argv[2];

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attr_name[0] == '/') {
		fprintf(stderr, "Invalid attribute name \"%s\" (cannot start "
			"with '/').\n", attr_name);
		goto out;
	}

	attrdirfd = attropen(
		path,
		".",
		O_RDONLY | O_NOFOLLOW);
	if(attrdirfd == -1) {
		fprintf(stderr, "Error while opening \"%s\" node's attribute "
			"directory: %s (%d)\n",
			path,
			strerror(errno),
			errno);
		goto out;
	}
#endif

#if defined(__APPLE__) || defined(__DARWIN__)
	if(removexattr(
		path,
		attr_name,
		0))
#elif defined(__linux__)
	if(lremovexattr(
		path,
		attr_name))
#elif defined(__FreeBSD__)
	if(extattr_delete_link(
		path,
		EXTATTR_NAMESPACE_USER,
		attr_name))
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(unlinkat(
		attrdirfd,
		attr_name,
		0))
#else
#error "Don't know how to handle extended attributes on this platform."
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	{
		fprintf(stderr, "Error while removing extended attribute: %s "
			"(errno=%d)\n",
			strerror(errno), errno);
		goto out;
	}

	ret = (EXIT_SUCCESS);
out:
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attrdirfd != -1) {
		close(attrdirfd);
	}
#endif

	return ret;
}
