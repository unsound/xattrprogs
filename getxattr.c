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

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#include <fcntl.h>
#include <unistd.h>
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/extattr.h>
#endif
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#include <sys/stat.h>
#endif
#if defined(__APPLE__) || defined(__DARWIN__) || defined(__linux__)
#include <sys/xattr.h>
#endif

int main(int argc, char **argv)
{
	int ret = (EXIT_FAILURE);
	int argp = 1;
#if defined(__FreeBSD__) || defined(__NetBSD__)
	int namespace = EXTATTR_NAMESPACE_USER;
#endif
	const char *path = NULL;;
	const char *attr_name = NULL;
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	int attrfd = -1;
	struct stat attrstat = { 0 };
#endif
	ssize_t attr_size = 0;
	char *attr_data = NULL;
	ssize_t bytes_read;

#if defined(__FreeBSD__) || defined(__NetBSD__)
	while(argp < argc) {
		if(argv[argp][0] != '-') {
			/* Not an option switch. Move on to the mandatory
			 * arguments. */
			break;
		}
		else if(argv[argp][1] == '-') {
			/* Stop parsing options when '--' is encountered. */
			++argp;
			break;
		}
#if defined(EXTATTR_NAMESPACE_EMPTY)
		else if(argv[argp][1] == 'e') {
			namespace = EXTATTR_NAMESPACE_EMPTY;
			++argp;
		}
#endif
		else if(argv[argp][1] == 'u') {
			namespace = EXTATTR_NAMESPACE_USER;
			++argp;
		}
		else if(argv[argp][1] == 's') {
			namespace = EXTATTR_NAMESPACE_SYSTEM;
			++argp;
		}
	}
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */

	path = (argp < argc) ? argv[argp++] : NULL;
	attr_name = (argp < argc) ? argv[argp++] : NULL;

	if(!path || !attr_name || argp < argc) {
		fprintf(stderr, "usage: getxattr "
#if defined(__FreeBSD__) || defined(__NetBSD__)
			"["
#if defined(EXTATTR_NAMESPACE_EMPTY)
			"-e|"
#endif
			"-u|-s] "
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */
			"<filename> <attribute name>"
			"\n");
		goto out;
	}

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attr_name[0] == '/') {
		fprintf(stderr, "Invalid attribute name \"%s\" (cannot start "
			"with '/').\n", attr_name);
		goto out;
	}

	attrfd = attropen(
		path,
		attr_name,
		O_RDONLY | O_NOFOLLOW);
	if(attrfd == -1) {
		fprintf(stderr, "Error while opening extended attribute \"%s\" "
			"of file \"%s\": %s (%d)\n",
			attr_name,
			path,
			strerror(errno),
			errno);
		goto out;
	}
#endif

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
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	attr_size = extattr_get_link(
		path,
		namespace,
		attr_name,
		NULL,
		0);
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(fstat(attrfd, &attrstat)) {
		attr_size = -1;
	}
	else {
		attr_size = attrstat.st_size;
	}
#else
#error "Don't know how to handle extended attributes on this platform."
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
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	bytes_read = extattr_get_link(
		path,
		namespace,
		attr_name,
		attr_data,
		attr_size);
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	bytes_read = read(
		attrfd,
		attr_data,
		attr_size);
#else
#error "Don't know how to handle extended attributes on this platform."
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
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attrfd != -1) {
		close(attrfd);
	}
#endif

	return ret;
}
