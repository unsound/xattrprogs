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

	if(argc != 3) {
		fprintf(stderr, "usage: removexattr <filename> "
			"<attribute name>\n");
		goto out;
	}

	path = argv[1];
	attr_name = argv[2];

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
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	{
		fprintf(stderr, "Error while removing extended attribute: %s "
			"(errno=%d)\n",
			strerror(errno), errno);
		goto out;
	}

	ret = (EXIT_SUCCESS);
out:
	return ret;
}
