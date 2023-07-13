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

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/extattr.h>
#endif
#if defined(__APPLE__) || defined(__DARWIN__) || defined(__linux__)
#include <sys/xattr.h>
#endif

int main(int argc, char **argv)
{
#if defined(__FreeBSD__) || defined(__NetBSD__)
	static const int namespaces[] = {
#ifdef EXTATTR_NAMESPACE_EMPTY
		EXTATTR_NAMESPACE_EMPTY,
#endif
		EXTATTR_NAMESPACE_USER,
		EXTATTR_NAMESPACE_SYSTEM,
	};
#endif

	int ret = (EXIT_FAILURE);
	int argp = 1;
#if defined(__FreeBSD__) || defined(__NetBSD__)
	int namespaces_start_index = 0;
	int namespaces_end_index = sizeof(namespaces) / sizeof(namespaces[0]);
	size_t i = 0;
#endif
	int follow_links = 0;
	const char *path = NULL;
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	int attrdirfd = -1;
#endif
	ssize_t attrlist_size = 0;
	char *attrlist = NULL;

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
			namespaces_start_index = 0;
			namespaces_end_index = 1;
			++argp;
		}
#endif
		else if(argv[argp][1] == 'u') {
			namespaces_start_index = 1;
			namespaces_end_index = 2;
			++argp;
		}
		else if(argv[argp][1] == 's') {
			namespaces_start_index = 2;
			namespaces_end_index = 3;
			++argp;
		}
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */
		else {
			fprintf(stderr, "Error: Unrecognized option '%s'.\n",
				argv[argp]);
			goto out;
		}
	}

	path = (argp < argc) ? argv[argp++] : NULL;

	if(!path || (argp < argc)) {
		fprintf(stderr, "usage: listxattr [-L"
#if defined(__FreeBSD__) || defined(__NetBSD__)
#ifdef EXTATTR_NAMESPACE_EMPTY
			"|-e"
#endif
			"|-u|-s"
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */
			"] <filename>\n");
		goto out;
	}

#if defined(__FreeBSD__) || defined(__NetBSD__)
	for(i = namespaces_start_index; i < namespaces_end_index; ++i)
#else
	do
#endif
	{
		ssize_t bytes_read = -1;
		ssize_t ptr = 0;

#if defined(__APPLE__) || defined(__DARWIN__)
		attrlist_size = listxattr(
			path,
			NULL,
			0,
			follow_links ? 0 : XATTR_NOFOLLOW);
#elif defined(__linux__)
		attrlist_size = (follow_links ? listxattr : llistxattr)(
			path,
			NULL,
			0);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
		attrlist_size =
			(follow_links ? extattr_list_file : extattr_list_link)(
				path,
				namespaces[i],
				NULL,
				0);
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
		int dup_fd = -1;
		DIR *dirp = NULL;
		struct dirent *de = NULL;
		int err = 0;
		int closedir_res = 0;

		attrdirfd = attropen(
			path,
			".",
			O_RDONLY | (follow_links ? 0 : O_NOFOLLOW));
		if(attrdirfd == -1) {
			fprintf(stderr, "Error while opening attribute "
				"directory of \"%s\": %s (%d)\n",
				path, strerror(errno), errno);
			goto out;
		}

		dup_fd = dup(attrdirfd);
		if(dup_fd == -1 || !(dirp = fdopendir(dup_fd))) {
			fprintf(stderr, "Error while getting attribute "
				"directory handle for \"%s\": %s (%d)\n",
				path, strerror(errno), errno);
			if(dup_fd != -1) {
				close(dup_fd);
			}

			goto out;
		}

		errno = 0;
		while((de = readdir(dirp))) {
			if(de->d_name[0] == '.' && (!de->d_name[1] ||
				(de->d_name[1] == '.' && !de->d_name[2])))
			{
				/* Ignore "." / "..". */
				continue;
			}

			attrlist_size += strlen(de->d_name) + 1;
			errno = 0;
		}

		err = errno;
		closedir_res = closedir(dirp);

		if(err) {
			fprintf(stderr, "Error while reading attribute "
				"directory of \"%s\": %s (%d)\n",
				path, strerror(err), err);
			goto out;
		}
		else if(closedir_res) {
			fprintf(stderr, "Error while closing attribute "
				"directory of \"%s\": %s (%d)\n",
				path, strerror(errno), errno);
			goto out;
		}
#else
#error "Don't know how to handle extended attributes on this platform."
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
		if(attrlist_size == 0) {
#ifdef DEBUG
			fprintf(stderr, "INFO: No extended attributes found "
				"for path \"%s\".\n", path);
#endif
			continue;
		}
		else if(attrlist_size == -1) {
#if defined(__FreeBSD__) || defined(__NetBSD__)
			if(errno == EPERM) {
				/* This is normal when a filesystem doesn't
				 * support a namespace. Just move on... */
				continue;
			}
#endif

			fprintf(stderr, "Error while getting size of extended "
				"attribute list "
#if defined(__FreeBSD__) || defined(__NetBSD__)
				"of namespace %d "
#endif
				"for path \"%s\": %s "
				"(errno=%d)\n",
#if defined(__FreeBSD__) || defined(__NetBSD__)
				namespaces[i],
#endif
				path, strerror(errno), errno);
			goto out;
		}

		if(attrlist) {
			free(attrlist);
		}

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
			follow_links ? 0 : XATTR_NOFOLLOW);
#elif defined(__linux__)
		bytes_read = (follow_links ? listxattr : llistxattr)(
			path,
			attrlist,
			attrlist_size);
#elif defined(__FreeBSD__) || defined(__NetBSD__)
		bytes_read =
			(follow_links ? extattr_list_file : extattr_list_link)(
				path,
				namespaces[i],
				attrlist,
				attrlist_size);
#elif (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
		{
			int dup_fd = -1;
			DIR *dirp = NULL;
			struct dirent *de = NULL;
			int err = 0;
			int closedir_res = 0;

			if(lseek(attrdirfd, 0, SEEK_SET)) {
				fprintf(stderr, "Error while seeking to start "
					"of directory: %s (%d)\n",
					strerror(errno), errno);
				goto out;
			}

			dup_fd = dup(attrdirfd);
			if(dup_fd == -1 || !(dirp = fdopendir(dup_fd))) {
				fprintf(stderr, "Error while getting attribute "
					"directory handle for \"%s\": %s "
					"(%d)\n",
					path, strerror(errno), errno);
				if(dup_fd != -1) {
					close(dup_fd);
				}

				goto out;
			}

			errno = 0;
			while((de = readdir(dirp))) {
				const size_t name_length = strlen(de->d_name);
				const size_t attrlist_remaining =
					attrlist_size - bytes_read;

				if(de->d_name[0] == '.' && (!de->d_name[1] ||
					(de->d_name[1] == '.' &&
					!de->d_name[2])))
				{
					/* Ignore "." / "..". */
					continue;
				}

				if(attrlist_remaining < name_length + 1) {
					fprintf(stderr, "Not enough space for "
						"all attributes in attribute "
						"list. List may have been "
						"modified behind our backs, "
						"please try again.\n");
					err = EINVAL;
					break;
				}

				strcpy(&attrlist[bytes_read], de->d_name);
				bytes_read += name_length + 1;
				errno = 0;
			}

			if(!de) {
				err = errno;
			}

			closedir_res = closedir(dirp);

			if(err) {
				fprintf(stderr, "Error while reading attribute "
					"directory of \"%s\": %s (%d)\n",
					path, strerror(err), err);
				goto out;
			}
			else if(closedir_res) {
				fprintf(stderr, "Error while closing attribute "
					"directory of \"%s\": %s (%d)\n",
					path, strerror(errno), errno);
				goto out;
			}
		}
#else
#error "Don't know how to handle extended attributes on this platform."
#endif /* defined(__APPLE__) || defined(__DARWIN__) ... */
		if(bytes_read < 0) {
			fprintf(stderr, "Error while reading extended "
				"attribute list "
#if defined(__FreeBSD__) || defined(__NetBSD__)
				"of namespace %d "
#endif
				"for path \"%s\": %s "
				"(errno=%d)\n",
#if defined(__FreeBSD__) || defined(__NetBSD__)
				namespaces[i],
#endif
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
#if defined(__FreeBSD__) || defined(__NetBSD__)
			char unknown_namespace_string[] =
				"<namespace -XXXXXXXXXX>";
			const char *namespace_string;
			char *cur = &attrlist[ptr + 1];
			unsigned char cur_len =
				*((unsigned char*) &attrlist[ptr]);
#else
			char *cur = &attrlist[ptr];
			int cur_len = strlen(cur);
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
			switch(namespaces[i]) {
#ifdef EXTATTR_NAMESPACE_EMPTY
			case EXTATTR_NAMESPACE_EMPTY:
				namespace_string = "";
				break;
#endif
			case EXTATTR_NAMESPACE_USER:
				namespace_string = "<user>";
				break;
			case EXTATTR_NAMESPACE_SYSTEM:
				namespace_string = "<system>";
				break;
			default:
				snprintf(unknown_namespace_string,
					sizeof(unknown_namespace_string),
					"<namespace %d>", namespaces[i]);
				namespace_string = unknown_namespace_string;
				break;
			}
#endif

			fprintf(stdout,
#if defined(__FreeBSD__) || defined(__NetBSD__)
				"%-*s "
#endif
				"%.*s\n",
#if defined(__FreeBSD__) || defined(__NetBSD__)
				(int) sizeof(unknown_namespace_string) - 1,
				namespace_string,
#endif
				cur_len, cur);

			ptr += cur_len + 1;
		}

		if(ptr != attrlist_size) {
			fprintf(stderr, "WARNING: ptr (%zd) != attrlist_size "
				"(%zd)\n",
				ptr, attrlist_size);
		}
	}
#if !(defined(__FreeBSD__) || defined(__NetBSD__))
	while(0);
#endif

	ret = (EXIT_SUCCESS);
out:
	if(attrlist) {
		free(attrlist);
	}

#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
	if(attrdirfd != -1) {
		close(attrdirfd);
	}
#endif

	return ret;
}
