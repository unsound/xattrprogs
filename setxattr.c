/*-
 * setxattr.c - Set an extended attribute for a filesystem node.
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
#include <stdint.h>

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#include <fcntl.h>
#endif
#include <unistd.h>
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/extattr.h>
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
	int follow_links = 0;
	const char *path;
	const char *attr_name = NULL;
	const char *attr_data = NULL;
#if defined(__APPLE__) || defined(__DARWIN__)
	const char *attr_offset_string = NULL;
	unsigned long long attr_offset = 0;
#endif
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	int nodefd = -1;
	int attrdirfd = -1;
	int attrfd = -1;
	ssize_t attr_bytes_written = 0;
#endif
	char *attr_data_alloc = NULL;
	size_t attr_data_size = 0;

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
		else if(argv[argp][1] == 'L') {
			follow_links = 1;
			++argp;
		}
#if defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef EXTATTR_NAMESPACE_EMPTY
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
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */
	}

	path = (argp < argc) ? argv[argp++] : NULL;
	attr_name = (argp < argc) ? argv[argp++] : NULL;
#if defined(__APPLE__) || defined(__DARWIN__)
	attr_offset_string = (argp < argc) ? argv[argp++] : NULL;
#endif
	attr_data = (argp < argc) ? argv[argp++] : NULL;

	if(!path || !attr_name || argp < argc) {
		fprintf(stderr, "usage: setxattr [-L"
#if defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef EXTATTR_NAMESPACE_EMPTY
			"|-e"
#endif
			"|-u|-s"
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */
			"] <filename> <attribute name> "
#if defined(__APPLE__) || defined(__DARWIN__)
			"[<attribute offset>] "
#endif
			"[<attribute data>]\n");
		goto out;
	}

#if defined(__APPLE__) || defined(__DARWIN__)
	if(attr_offset_string) {
		char *endptr = NULL;

		errno = 0;
		attr_offset = strtoull(attr_offset_string, &endptr, 0);
		if(errno || ((*endptr || attr_offset > SIZE_MAX) &&
			(errno = EILSEQ)))
		{
			fprintf(stderr, "Invalid offset: %s\n",
				attr_offset_string);
			goto out;
		}
	}
#endif

	if(attr_data) {
		attr_data_size = strlen(attr_data);
	}
	else {
		size_t alloc_size = 4096;

		attr_data = NULL;
		attr_data_size = 0;

		while(1) {
			char *new_attr_data;
			ssize_t bytes_read;

			new_attr_data = realloc(attr_data_alloc, alloc_size);
			if(!new_attr_data) {
				fprintf(stderr, "Error while %sallocating "
					"attribute buffer to %zu bytes: %s "
					"(errno=%d)\n",
					attr_data ? "" : "re", alloc_size,
					strerror(errno), errno);
				goto out;
			}

			attr_data_alloc = new_attr_data;
			attr_data = attr_data_alloc;

			bytes_read = read(STDIN_FILENO, 
				&attr_data_alloc[attr_data_size],
				alloc_size - attr_data_size);
			if(bytes_read < 0) {
				fprintf(stderr, "Error while reading xattr "
					"data from stdin: %s (errno=%d)\n",
					strerror(errno), errno);
				goto out;
			}

			if(!bytes_read) {
				break;
			}

			attr_data_size += bytes_read;
			alloc_size *= 2;
		}

		if(alloc_size != attr_data_size) {
			char *new_attr_data;

			new_attr_data =
				realloc(attr_data_alloc, attr_data_size);
			if(!new_attr_data) {
				fprintf(stderr, "Error while shrinking "
					"attribute buffer from %zu to %zu "
					"bytes: %s (errno=%d)\n",
					alloc_size, attr_data_size,
					strerror(errno), errno);
				goto out;
			}

			attr_data_alloc = new_attr_data;
			attr_data = attr_data_alloc;
		}
	}

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attr_name[0] == '/') {
		fprintf(stderr, "Invalid attribute name \"%s\" (cannot start "
			"with '/').\n", attr_name);
		goto out;
	}

	nodefd = open(
		path,
		O_RDONLY | (follow_links ? 0 : O_NOFOLLOW));
	if(nodefd == -1) {
		fprintf(stderr, "Error while opening node \"%s\": %s (%d)\n",
			path,
			strerror(errno),
			errno);
		goto out;
	}

	attrdirfd = openat(nodefd, ".", O_RDONLY | O_XATTR);
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
	if(setxattr(
		path,
		attr_name,
		attr_data,
		attr_data_size,
		attr_offset,
		follow_links ? 0 : XATTR_NOFOLLOW))
#elif defined(__linux__)
	if((follow_links ? setxattr : lsetxattr)(
		path,
		attr_name,
		attr_data,
		attr_data_size,
		0))
#elif defined(__FreeBSD__) || defined(__NetBSD__)
	if((follow_links ? extattr_set_file : extattr_set_link)(
		path,
		namespace,
		attr_name,
		attr_data,
		attr_data_size) < 0)
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	/* TODO: Solaris allows whole xattr directory hierarchies under a node.
	 * To support creating attributes in xattr directory hierarchies we must
	 * split any pathname components and create directories for them if they
	 * do not exist. At the moment setting such xattrs will fail. */

	/* If the attribute existed before, then we should remove it. */
	if(unlinkat(attrdirfd, attr_name, 0) && errno != ENOENT) {
		fprintf(stderr, "Error while removing existing extended "
			"attribute  \"%s\" for node \"%s\": %s (%d)\n",
			attr_name, path, strerror(errno), errno);
		goto out;
	}

	attrfd = openat(nodefd, attr_name,
		O_WRONLY | O_CREAT | O_EXCL | O_XATTR, 0777);
	if(attrfd == -1) {
		fprintf(stderr, "Error while creating extended attribute "
			"\"%s\" for node \"%s\": %s (%d)\n",
			attr_name, path, strerror(errno), errno);
		goto out;
	}

	/* Write out the attribute data. */
	attr_bytes_written = write(attrfd, attr_data, attr_data_size);
	if(attr_bytes_written >= 0 && attr_bytes_written != attr_data_size) {
		fprintf(stderr, "Partial write while setting extended "
			"attribute \"%s\" for node \"%s\": %zu / %zu bytes "
			"written\n",
			attr_name, path, attr_bytes_written, attr_data_size);
		goto out;
	}
	else if(attr_bytes_written < 0)
#else
#error "Don't know how to handle extended attributes on this platform."
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
	{
		fprintf(stderr, "Failed to set extended attribute: %s "
			"(errno=%d)\n",
			strerror(errno), errno);
		goto out;
	}

	ret = (EXIT_SUCCESS);
out:
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attrfd != -1) {
		close(attrfd);
	}
#endif

	if(attr_data_alloc) {
		free(attr_data_alloc);
	}

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attrdirfd != -1) {
		close(attrdirfd);
	}

	if(nodefd != -1) {
		close(nodefd);
	}
#endif

	return ret;
}
